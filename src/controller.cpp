#include "controller.h"

#include "config.h"
#include "storage.h"
#include "utils.h"

void Controller::begin(Scale* sc, Encoder* enc, Buttons* btn, Display* disp,
                       Relay* rel) {
    sc_ = sc;
    enc_ = enc;
    btn_ = btn;
    disp_ = disp;
    rel_ = rel;
}

void Controller::update() {
    // --- update peripherals ---
    sc_->update();
    enc_->update();
    btn_->update();

    static uint32_t hintUntil = 0;  // transient UI hint window

    // --- long-press: enter/advance calibration (requires stability) ---
    static int32_t cal_raw0 = 0;  // persistent across calls
    static uint32_t calDoneUntil = 0;

    if (enc_->tareLongPressed()) {
        if (REQUIRE_STABLE_FOR_CAL && !sc_->isStable()) {
            hintUntil = millis() + HINT_HOLD_MS;  // show HOLD
        } else if (state_ == AppState::IDLE ||
                   state_ == AppState::SHOW_SETPOINT) {
            // Capture zero point and move to span prompt
            cal_raw0 = sc_->rawNoTare();
            state_ = AppState::CAL_SPAN;
        } else if (state_ == AppState::CAL_SPAN) {
            // Capture span point, compute factor (Q16): mg_per_count_q16 =
            // (span_mg << 16) / dcounts
            int32_t raw1 = sc_->rawNoTare();
            int32_t dcounts = raw1 - cal_raw0;
            if (dcounts == 0) dcounts = 1;
            int32_t span_mg = lround_mg(CAL_SPAN_MASS_G);
            int64_t num = ((int64_t)span_mg) << 16;
            int32_t mg_per_count_q16 =
                (int32_t)(num / (int64_t)((dcounts > 0) ? dcounts : -dcounts));
            if (mg_per_count_q16 <= 0)
                mg_per_count_q16 = CAL_MG_PER_COUNT_Q16;  // fallback
            sc_->setCalMgPerCountQ16(mg_per_count_q16);
            storage::saveCalQ16(mg_per_count_q16);
            // brief done screen
            state_ = AppState::DONE_HOLD;
            calDoneUntil = millis() + DONE_HOLD_MS;
        }
    }

    // --- handle encoder setpoint ---
    int32_t dmg = enc_->consumeDeltaMg();
    if (dmg != 0) {
        int32_t maxMg = lround_mg(SETPOINT_MAX_G);
        setpoint_mg_ = clamp_i32(setpoint_mg_ + dmg, 0, maxMg);
        tShowUntil_ = millis() + SHOW_SP_MS;
        if (state_ == AppState::IDLE) state_ = AppState::SHOW_SETPOINT;
    }

    // --- save setpoint when user stops turning (on timeout exit) ---
    if (state_ == AppState::SHOW_SETPOINT && millis() > tShowUntil_) {
        state_ = AppState::IDLE;
        storage::saveSetpointMg(setpoint_mg_);
    }

    // --- tare (short press) ---
    if (enc_->tarePressed()) {
        bool measuring = (state_ == AppState::MEASURING);
        if (measuring) {
            hintUntil = millis() + HINT_HOLD_MS;  // blocked during measuring
        } else if (!REQUIRE_STABLE_FOR_TARE || sc_->isStable()) {
            sc_->tare();
            storage::saveTareRaw(sc_->tareRaw());
        } else {
            hintUntil = millis() + HINT_HOLD_MS;  // ask user to hold still
        }
    }

    // --- start/stop ---
    if (btn_->startPressed()) {
        if (state_ == AppState::MEASURING) {
            rel_->set(false);
            state_ = AppState::DONE_HOLD;
            tDoneUntil_ = millis() + DONE_HOLD_MS;
        } else if (state_ == AppState::IDLE ||
                   state_ == AppState::SHOW_SETPOINT) {
            // start
            rel_->set(true);
            state_ = AppState::MEASURING;
            tMeasureUntil_ = millis() + MEASURE_TIMEOUT_MS;
        } else if (state_ == AppState::CAL_SPAN) {
            // allow abort of calibration with start button
            state_ = AppState::IDLE;
        }
    }

    // --- state machine ---
    switch (state_) {
        case AppState::IDLE:
            break;
        case AppState::SHOW_SETPOINT:
            // handled above for timeout/persist
            break;
        case AppState::MEASURING: {
            int32_t effective = setpoint_mg_ - CUTOFF_OFFSET_MG;
            if (sc_->filteredMg() + HYSTERESIS_MG >= effective ||
                millis() > tMeasureUntil_) {
                rel_->set(false);
                state_ = AppState::DONE_HOLD;
                tDoneUntil_ = millis() + DONE_HOLD_MS;
            }
        } break;
        case AppState::DONE_HOLD:
            if (millis() > tDoneUntil_) state_ = AppState::IDLE;
            break;
        case AppState::CAL_SPAN:
            // stay here until second long-press or abort; just show prompt
            break;
        case AppState::ERROR_STATE:
            break;
        case AppState::CAL_ZERO:
            state_ = AppState::CAL_SPAN;  // unused placeholder
            break;
    }

    // --- display (hint overlay) ---
    if (millis() < hintUntil) {
        disp_->showHintHold();
        return;
    }

    // --- display ---
    if (!sc_->ok()) {
        disp_->showError();
    } else if (state_ == AppState::SHOW_SETPOINT) {
        disp_->showSetpointMg(setpoint_mg_);
    } else if (state_ == AppState::CAL_SPAN) {
        disp_->showCalSpan();
    } else if (state_ == AppState::DONE_HOLD && millis() < calDoneUntil) {
        disp_->showCalDone();
    } else if (state_ == AppState::DONE_HOLD || state_ == AppState::MEASURING ||
               state_ == AppState::IDLE) {
        disp_->showWeightMg(sc_->filteredMg(), sc_->isStable());
    } else {
        disp_->showError();
    }
}
