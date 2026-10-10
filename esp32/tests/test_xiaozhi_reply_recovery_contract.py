#!/usr/bin/env python3
"""Regression contracts for the Xiaozhi reply UI and AP recovery fixes."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class XiaozhiReplyRecoveryContract(unittest.TestCase):
    def test_xiaozhi_console_uses_ch340_uart(self):
        kconfig = (ROOT / "components" / "muse" / "Kconfig").read_text(encoding="utf-8")
        self.assertRegex(
            kconfig,
            r"default y if[^\n]*MUSE_BOARD_XIAOZHI_154",
            "Xiaozhi bench commands must use the CH340-backed console UART",
        )

    def test_xiaozhi_overlay_builds_cjk_glyphs(self) -> None:
        overlay = (ROOT / "devices" / "sdkconfig.muse-xiaozhi-154").read_text(encoding="utf-8")
        self.assertIn("CONFIG_MUSE_CJK_FONT=y", overlay)

    def test_xiaozhi_filters_release_bounce_repress(self) -> None:
        board = (ROOT / "components" / "muse" / "boards" / "board_xiaozhi_154.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("TALK_REPRESS_GUARD_MS 150", board)
        self.assertIn("s_talk_repress_ignored = true;", board)
        self.assertIn("ev &= ~MUSE_BTN_TALK_RELEASE;", board)

    def test_pairing_turns_wifi_back_on(self) -> None:
        app = (ROOT / "main" / "app.c").read_text(encoding="utf-8")
        complete = app[app.index("static bool complete_setup_and_stop_ble(") :]
        complete = complete[: complete.index("static void restart_task")]
        self.assertIn("muse_settings_set_wifi_on(true);", complete)

    def test_text_only_reply_uses_reading_layout_and_wakes_screen(self) -> None:
        ui = (ROOT / "components" / "muse" / "muse_ui.c").read_text(encoding="utf-8")
        voice = (ROOT / "components" / "muse" / "muse_voice.c").read_text(encoding="utf-8")
        state = (ROOT / "components" / "muse" / "muse_state.h").read_text(encoding="utf-8")
        session = (ROOT / "components" / "muse" / "muse_chat_session.cpp").read_text(encoding="utf-8")
        self.assertIn("int layout = ANSWER_READ;", ui)
        self.assertIn("build_small_answer(face);", ui)
        self.assertIn("l->hides[0] = s_canvas;", ui)
        self.assertIn("muse_state_set_cjk_page(page_w / 16, lines);", ui)
        self.assertIn('muse_input_wake("Muse reply")', voice)
        self.assertIn("#define MUSE_CAPTION_MAX 512", state)
        self.assertIn("#define TEXT_HOLD_S 5", session)

    def test_compact_reader_uses_aux_for_navigation(self) -> None:
        input_c = (ROOT / "components" / "muse" / "muse_input.c").read_text(encoding="utf-8")
        voice_c = (ROOT / "components" / "muse" / "muse_voice.c").read_text(encoding="utf-8")
        session = (ROOT / "components" / "muse" / "muse_chat_session.cpp").read_text(encoding="utf-8")
        self.assertIn("muse_voice_reply_next();", input_c)
        self.assertIn("muse_voice_reply_dismiss();", input_c)
        self.assertIn("REPLY_NAV_NEXT", voice_c)
        self.assertIn("REPLY_NAV_DISMISS", voice_c)
        self.assertIn("shown_chars * MIC_RATE / TEXT_CHARS_PER_S", session)

    def test_compact_reader_has_independent_page_indicator(self) -> None:
        ui = (ROOT / "components" / "muse" / "muse_ui.c").read_text(encoding="utf-8")
        text = (ROOT / "components" / "muse" / "muse_chat_text.c").read_text(encoding="utf-8")
        self.assertIn("s_reply_page_lbl", ui)
        self.assertIn('lv_label_set_text_fmt(s_reply_page_lbl, "%d/%d", page, pages);', ui)
        self.assertIn("pages > 1", ui)
        self.assertIn("muse_state_set_reply_page(first / step + 1, pages);", text)

    def test_restored_wifi_clears_stale_dns_requests(self) -> None:
        wifi = (ROOT / "main" / "wifi_mgr.c").read_text(encoding="utf-8")
        disconnected = wifi.index("wifi disconnected post-association")
        recovered = wifi.index("network restored; cleared stale DNS requests")
        self.assertLess(disconnected, recovered)
        self.assertIn("s_recovering = true;", wifi)
        self.assertIn("dns_clear_cache();", wifi)
        self.assertIn("LOCK_TCPIP_CORE();", wifi)
        self.assertIn("UNLOCK_TCPIP_CORE();", wifi)

    def test_aux_hold_provides_two_button_menu_back(self) -> None:
        input_c = (ROOT / "components" / "muse" / "muse_input.c").read_text(encoding="utf-8")
        self.assertIn("MENU_BACK_TICKS", input_c)
        self.assertIn("muse_menu_key(MUSE_MENU_BACK);", input_c)

    def test_voice_turn_cannot_be_replaced_by_settings_menu(self) -> None:
        input_c = (ROOT / "components" / "muse" / "muse_input.c").read_text(encoding="utf-8")
        voice = (ROOT / "components" / "muse" / "muse_voice.c").read_text(encoding="utf-8")
        menu = (ROOT / "components" / "muse" / "muse_menu.c").read_text(encoding="utf-8")
        self.assertIn("else if (mode == MUSE_MODE_IDLE)", input_c)
        self.assertIn('aux key ignored during voice turn', input_c)
        self.assertIn("muse_state_mode(NULL) != MUSE_MODE_IDLE", input_c)
        self.assertGreaterEqual(voice.count("muse_menu_request_close();"), 2)
        self.assertIn("__atomic_exchange_n(&s_close_requested", menu)

    def test_approval_command_card_and_buttons_are_wired_together(self) -> None:
        register = (ROOT / "main" / "noise_control.cpp").read_text(encoding="utf-8")
        app = (ROOT / "main" / "app.c").read_text(encoding="utf-8")
        ui = (ROOT / "components" / "muse" / "muse_ui.c").read_text(encoding="utf-8")
        input_c = (ROOT / "components" / "muse" / "muse_input.c").read_text(encoding="utf-8")
        approval = (ROOT / "components" / "muse" / "muse_approval.c").read_text(encoding="utf-8")
        self.assertIn('commands, "approval.request"', register)
        self.assertIn('strcmp(command, "approval.request")', app)
        self.assertIn('muse_input_wake("approval request")', app)
        self.assertIn("muse_approval_wait_visible", app)
        self.assertIn('"not_displayed"', app)
        self.assertIn('lv_label_set_text(approve, "TALK\\nAPPROVE")', ui)
        self.assertIn('lv_label_set_text(deny, "VOLUME\\nDENY")', ui)
        self.assertIn("if (update_approval())", ui)
        self.assertIn("muse_approval_mark_visible(version);", ui)
        self.assertIn("if (muse_approval_visible())", input_c)
        self.assertIn("muse_approval_respond(true)", input_c)
        self.assertIn("muse_approval_respond(false)", input_c)
        self.assertIn("MUSE_APPROVAL_TIMED_OUT", approval)

    def test_typed_reply_barge_in_reuses_the_same_talk_press(self) -> None:
        voice = (ROOT / "components" / "muse" / "muse_voice.c").read_text(encoding="utf-8")
        self.assertIn('voice_note ? "SENDING VOICE NOTE" : "PREPARING REPLY"', voice)
        self.assertIn("pending_down = hatch_reply(&delivered, false);", voice)
        self.assertIn("if (!pending_down) {\n                    continue;", voice)

    def test_barge_in_preserves_release_queued_behind_press(self) -> None:
        voice = (ROOT / "components" / "muse" / "muse_voice.c").read_text(encoding="utf-8")
        self.assertIn("static bool s_deferred_ptt_up;", voice)
        self.assertIn("type == MUSE_PTT_DOWN && ev.type == MUSE_PTT_UP", voice)
        self.assertIn("__atomic_exchange_n(&s_deferred_ptt_up", voice)

    def test_long_text_reply_does_not_wait_for_tts_to_render(self) -> None:
        voice = (ROOT / "components" / "muse" / "muse_voice.c").read_text(encoding="utf-8")
        session = (ROOT / "components" / "muse" / "muse_chat_session.cpp").read_text(encoding="utf-8")
        self.assertIn("!voice_note && muse_hatch_turn_caption(0, page, sizeof(page))", voice)
        self.assertIn('ESP_LOGW(TAG, "reply audio did not start within 15s")', voice)
        self.assertIn("s_turn.playback_requested = true;", session)
        self.assertIn("emit(MUSE_HATCH_EV_REPLY, nullptr);", session)

    def test_sleeping_voice_turn_waits_for_busy_agent_then_wakes_on_reply(self) -> None:
        input_c = (ROOT / "components" / "muse" / "muse_input.c").read_text(encoding="utf-8")
        voice = (ROOT / "components" / "muse" / "muse_voice.c").read_text(encoding="utf-8")
        session = (ROOT / "components" / "muse" / "muse_chat_session.cpp").read_text(encoding="utf-8")
        self.assertIn("mode == MUSE_MODE_IDLE || mode == MUSE_MODE_THINKING", input_c)
        self.assertIn("int64_t reply_started_us = 0;", voice)
        self.assertIn("reply_started_us && !speaking && !done", voice)
        self.assertIn("esp_timer_get_time() - reply_started_us > 15 * 1000000LL", voice)
        self.assertNotIn("esp_timer_get_time() - t0 > 15 * 1000000LL", voice)
        self.assertIn("&& !s_turn.agent_busy", session)
        self.assertNotIn("!(text && s_turn.agent_busy)", session)
        self.assertIn('muse_input_wake("Muse reply")', voice)

    def test_serial_commands_cannot_turn_truncated_text_into_button_events(self) -> None:
        input_c = (ROOT / "components" / "muse" / "muse_input.c").read_text(encoding="utf-8")
        echo = (ROOT / "tools" / "muse" / "echo_turn.py").read_text(encoding="utf-8")
        snap = (ROOT / "tools" / "muse" / "snap.py").read_text(encoding="utf-8")
        serial = input_c[input_c.index("static void serial_task") : input_c.index("esp_err_t muse_input_start")]
        self.assertIn('!strncmp(line, "key=", 4)', input_c)
        self.assertNotIn("if (c == 'm')", serial)
        self.assertNotIn("c == 'd' || c == 'u'", serial)
        self.assertIn("b'>key=d\\n'", echo)
        self.assertIn("b'>key=u\\n'", echo)
        self.assertIn("f'>key={k}\\n'", snap)
        self.assertIn("b'>key=p\\n'", snap)


if __name__ == "__main__":
    unittest.main()
