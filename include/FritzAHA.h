#pragma once
#include <Arduino.h>

/*
  Minimal FRITZ!Box AHA client
  - Login via /login_sid.lua?version=2 (PBKDF2 v2 or MD5 legacy)
  - Generic aha(): GET /webservices/homeautoswitch.lua
  - Helpers: switch_on / switch_off / state

  Notes:
  * Pass AINs WITHOUT spaces (this header normalizes by removing spaces).
  * For HTTPS you’ll need WiFiClientSecure and certificate handling (not shown).
*/

class FritzAHA {
   public:
    FritzAHA(const String& base, const String& user, const String& pass)
        : _base(base), _user(user), _pass(pass) {}

    // Get a valid 16-char SID, or "" on failure.
    String login();

    // -------- Generic AHA call (returns raw text; trims) --------
    // switchcmd = e.g. "setswitchon", "getswitchstate"
    // ain       = device AIN (spaces will be removed); optional for list/info
    // cmds param     = optional extra parameter for commands that require it
    String aha(const String& sid, const String& switchcmd,
               const String& ain = "", const String& param = "");

    // -------- Convenience wrappers --------
    bool switch_on(const String& sid, const String& ain);
    bool switch_off(const String& sid, const String& ain);
    bool switch_toggle(const String& sid, const String& ain);
    int state(const String& sid, const String& ain);
    int getswitchpower(const String& sid, const String& ain);

   private:
    String _base, _user, _pass;
};
