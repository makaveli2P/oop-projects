#include "strset_tui/keys.h"

#include <stdexcept>

#include "strset/command.h"
#include "strset_tui/format.h"

namespace strset::tui {

namespace {

bool is_enter(std::string_view key) { return key == "Enter" || key == "\n" || key == "\r"; }

KeyResult prompt_key(Controller& c, std::string_view key) {
    if (is_enter(key)) {
        c.submit_prompt();
    } else if (key == "Escape") {
        c.cancel_prompt();
    } else if (key == "Backspace") {
        c.erase_last();
    } else if (display_width(key) == 1 && static_cast<unsigned char>(key.front()) >= 0x20 && key != "\x7f") {
        c.type(std::string{key});
    } else {
        return KeyResult::Ignored;
    }
    return KeyResult::Handled;
}

}  // namespace

KeyResult handle_key(Controller& c, std::string_view key) {
    if (key.empty()) {
        return KeyResult::Ignored;
    }
    if (c.prompt_open()) {
        return prompt_key(c, key);
    }
    if (c.help_open()) {
        c.close_help();
        if (key == "q") {
            c.pause();
            return KeyResult::Quit;
        }
        return KeyResult::Handled;
    }
    if (key.size() == 1) {
        CommandKind kind{};
        if (command_for_letter(key.front(), kind)) {
            switch (kind) {
                case CommandKind::Add: c.open_prompt(); return KeyResult::Handled;
                case CommandKind::Help: c.toggle_help(); return KeyResult::Handled;
                case CommandKind::Quit: c.pause(); return KeyResult::Quit;
                default: c.start(Command{kind, {}}); return KeyResult::Handled;
            }
        }
    }
    if (key == " " || key == "Right" || is_enter(key)) {
        c.next();
    } else if (key == "Left") {
        c.back();
    } else if (key == "r") {
        c.toggle_run();
    } else if (key == "g" || key == "End") {
        c.to_end();
    } else if (key == "+" || key == "=") {
        c.faster();
    } else if (key == "-" || key == "_") {
        c.slower();
    } else if (key == "Escape") {
        c.pause();
        return KeyResult::Quit;
    } else {
        return KeyResult::Ignored;
    }
    return KeyResult::Handled;
}

std::vector<std::string> parse_key_script(std::string_view script) {
    std::vector<std::string> keys;
    std::size_t i = 0;
    while (i < script.size()) {
        if (script[i] == '{') {
            const std::size_t close = script.find('}', i);
            if (close == std::string_view::npos) {
                throw std::invalid_argument("a '{' without '}' in the key script");
            }
            const std::string_view name = script.substr(i + 1, close - i - 1);
            if (name == "enter") keys.emplace_back("Enter");
            else if (name == "esc") keys.emplace_back("Escape");
            else if (name == "bs") keys.emplace_back("Backspace");
            else if (name == "left") keys.emplace_back("Left");
            else if (name == "right") keys.emplace_back("Right");
            else if (name == "end") keys.emplace_back("End");
            else if (name == "space") keys.emplace_back(" ");
            else throw std::invalid_argument("unknown key name {" + std::string{name} + "}");
            i = close + 1;
            continue;
        }
        std::size_t end = i + 1;
        while (end < script.size() && (static_cast<unsigned char>(script[end]) & 0xC0U) == 0x80U) ++end;
        keys.emplace_back(script.substr(i, end - i));
        i = end;
    }
    return keys;
}

}  // namespace strset::tui
