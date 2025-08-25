#include "FritzAHA.h"

#include <HTTPClient.h>

#include <vector>

#include "mbedtls/md.h"
#include "mbedtls/md5.h"
#include "mbedtls/pkcs5.h"

// ---------- Small utils ----------
static bool isValidSid(const String& sid) {
    return (sid.length() == 16 && sid != "0000000000000000");
}

static String normalize_ain(const String& ain) {
    String out;
    out.reserve(ain.length());
    for (size_t i = 0; i < ain.length(); ++i)
        if (ain[i] != ' ') out += ain[i];
    return out;
}

static String urlEncode(const String& s) {
    String out;
    out.reserve(s.length() * 3);
    const char* hex = "0123456789ABCDEF";
    for (size_t i = 0; i < s.length(); ++i) {
        unsigned char c = (unsigned char)s[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            out += (char)c;
        else if (c == ' ')
            out += '+';
        else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 15];
        }
    }
    return out;
}

static String toHex(const uint8_t* buf, size_t n) {
    const char* h = "0123456789abcdef";
    String s;
    s.reserve(n * 2);
    for (size_t i = 0; i < n; i++) {
        s += h[buf[i] >> 4];
        s += h[buf[i] & 15];
    }
    return s;
}

static String httpGet(const String& url) {
    HTTPClient http;
    http.begin(url);
    int code = http.GET();
    String s = (code > 0) ? http.getString() : "";
    http.end();
    return (code == 200) ? s : "";
}

static String httpPostForm(const String& url, const String& body) {
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int code = http.POST(body);
    String s = (code > 0) ? http.getString() : "";
    http.end();
    return (code == 200) ? s : "";
}

static String parseXml(const String& xml, const String& tag) {
    String a = "<" + tag + ">", b = "</" + tag + ">";
    int i = xml.indexOf(a), j = xml.indexOf(b);
    return (i >= 0 && j > i) ? xml.substring(i + a.length(), j) : "";
}

// ---------- Auth: MD5 legacy (UTF-16LE) ----------
static String md5_legacy_response(const String& challenge,
                                  const String& password) {
    String comb = challenge + "-" + password;
    std::vector<uint8_t> u16;
    u16.reserve(comb.length() * 2);
    for (size_t i = 0; i < comb.length(); ++i) {
        uint16_t cp = (uint8_t)comb[i];
        if (cp > 255) cp = '.';
        u16.push_back((uint8_t)cp);
        u16.push_back(0x00);
    }
    unsigned char md5[16];
    mbedtls_md5(u16.data(), u16.size(), md5);
    return challenge + "-" + toHex(md5, 16);
}

// ---------- Auth: PBKDF2 v2 (double SHA-256) ----------
static String pbkdf2_response(const String& challenge, const String& password) {
    // 2$<iter1>$<salt1hex>$<iter2>$<salt2hex>
    int p1 = challenge.indexOf('$');
    int p2 = challenge.indexOf('$', p1 + 1);
    int p3 = challenge.indexOf('$', p2 + 1);
    int p4 = challenge.indexOf('$', p3 + 1);
    uint32_t iter1 = challenge.substring(p1 + 1, p2).toInt();
    String salt1 = challenge.substring(p2 + 1, p3);
    uint32_t iter2 = challenge.substring(p3 + 1, p4).toInt();
    String salt2 = challenge.substring(p4 + 1);

    auto hex2bin = [](const String& hx) {
        std::vector<uint8_t> v;
        v.reserve(hx.length() / 2);
        for (int i = 0; i + 1 < hx.length(); i += 2)
            v.push_back(strtoul(hx.substring(i, i + 2).c_str(), nullptr, 16));
        return v;
    };
    auto s1 = hex2bin(salt1), s2 = hex2bin(salt2);

    uint8_t h1[32], h2[32];
    mbedtls_pkcs5_pbkdf2_hmac_ext(
        MBEDTLS_MD_SHA256, (const uint8_t*)password.c_str(), password.length(),
        s1.data(), s1.size(), iter1, 32, h1);
    mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256, h1, 32, s2.data(),
                                  s2.size(), iter2, 32, h2);

    return salt2 + "$" + toHex(h2, 32);
}

// Get a valid 16-char SID, or "" on failure.
String FritzAHA::login() {
    String xml = httpGet(_base + "/login_sid.lua?version=2");
    String sid = parseXml(xml, "SID");
    if (isValidSid(sid)) return sid;

    String challenge = parseXml(xml, "Challenge");
    if (challenge.length() == 0) return "";
    String response = challenge.startsWith("2$")
                          ? pbkdf2_response(challenge, _pass)
                          : md5_legacy_response(challenge, _pass);
    String body =
        "username=" + urlEncode(_user) + "&response=" + urlEncode(response);
    String xml2 = httpPostForm(_base + "/login_sid.lua?version=2", body);
    String sid2 = parseXml(xml2, "SID");
    return isValidSid(sid2) ? sid2 : "";
}

// -------- Generic AHA call (returns raw text; trims) --------
// switchcmd = e.g. "setswitchon", "getswitchstate"
// ain       = device AIN (spaces will be removed); optional for list/info cmds
// param     = optional extra parameter for commands that require it
String FritzAHA::aha(const String& sid, const String& switchcmd,
                     const String& ain, const String& param) {
    String url = _base + "/webservices/homeautoswitch.lua?switchcmd=" +
                 urlEncode(switchcmd) + "&sid=" + urlEncode(sid);
    if (ain.length()) url += "&ain=" + urlEncode(normalize_ain(ain));
    if (param.length()) url += "&param=" + urlEncode(param);
    String resp = httpGet(url);
    resp.trim();
    return resp;
}

// -------- Convenience wrappers --------
bool FritzAHA::switch_on(const String& sid, const String& ain) {
    return aha(sid, "setswitchon", ain) == "1";
}

bool FritzAHA::switch_off(const String& sid, const String& ain) {
    return aha(sid, "setswitchoff", ain) == "0";
}

bool FritzAHA::switch_toggle(const String& sid, const String& ain) {
    return aha(sid, "setswitchtoggle", ain) == "0";
}

int FritzAHA::state(const String& sid, const String& ain) {
    String r = aha(sid, "getswitchstate", ain);
    if (r == "0" || r == "1") return r.toInt();
    return -1;  // error
}

int FritzAHA::getswitchpower(const String& sid, const String& ain) {
    String r = aha(sid, "getswitchpower", ain);
    return r.length() ? r.toInt() : -1;  // mW, -1 on error
}
