#include <sstream>
#include <boost/algorithm/string.hpp>

#include "dbat/game/EditorMode.hpp"
#include "dbat/game/Descriptor.hpp"
#include "dbat/game/CharacterUtils.hpp"


EditorMode::EditorMode(descriptor_data& desc, size_t maxLength)
    : DescriptorMode(desc), maxLength_(maxLength) {
}

void EditorMode::launch() {
}

void EditorMode::cleanup() {
}

bool EditorMode::validate() {
    return true;
}

void EditorMode::save() {
}

void EditorMode::abort() {
}

void EditorMode::displayBuffer() {
    desc.sendFmt("{}\r\n", workingBuffer_);
}

bool EditorMode::isValid() {
    return true;
}

void EditorMode::onLaunch() {
    desc.sendText("Instructions: /s to save, /h for more options.\r\n");
    launch();
    if (!workingBuffer_.empty()) {
        displayBuffer();
    }
}

void EditorMode::onClose() {
    cleanup();
}

void EditorMode::onReplace() {
    onClose();
}

bool EditorMode::isModeValid() {
    if (!isValid()) {
        desc.sendText("\r\nThe target of your edit has been deleted. Aborting.\r\n");
        return false;
    }
    return true;
}

void EditorMode::handleInput(std::string_view input) {
    std::string processedInput{input};
    
    if (!processedInput.empty() && processedInput[0] == '/') {
        auto result = processCommand(processedInput);
        if (result == EditorResult::Save) {
            if (validate()) {
                save();
                desc.mode.reset();
            }
        } else if (result == EditorResult::Abort) {
            abort();
            desc.mode.reset();
        }
        return;
    }
    
    if (!workingBuffer_.empty()) {
        workingBuffer_ += "\r\n";
    }
    
    size_t newLength = workingBuffer_.length() + processedInput.length() + 3;
    if (newLength > maxLength_) {
        desc.sendText("String too long - Truncated.\r\n");
        workingBuffer_ = workingBuffer_.substr(0, maxLength_ - 3);
        return;
    }
    
    workingBuffer_ += processedInput;

}

EditorResult EditorMode::processCommand(std::string_view input) {
    if (input.length() < 2 || input[0] != '/') {
        return EditorResult::Continue;
    }
    
    std::string actions;
    if (input.length() > 2) {
        actions = std::string(input.substr(2));
    }
    
    char command = input[1];
    
    switch (tolower(command)) {
        case 'a':
            return EditorResult::Abort;
        case 'c':
            if (!workingBuffer_.empty()) {
                workingBuffer_.clear();
                desc.sendText("Current buffer cleared.\r\n");
            } else {
                desc.sendText("Current buffer empty.\r\n");
            }
            break;
        case 'd':
            return parseDelete(actions);
        case 'e':
            return parseEdit(actions);
        case 'f':
            if (!workingBuffer_.empty()) {
                return parseFormat(actions);
            } else {
                desc.sendText("Current buffer empty.\r\n");
            }
            break;
        case 'h':
            desc.sendText(
                "Editor command formats: /<letter>\r\n\r\n"
                "/a         -  aborts editor\r\n"
                "/c         -  clears buffer\r\n"
                "/d#        -  deletes a line #\r\n"
                "/e# <text> -  changes the line at # with <text>\r\n"
                "/f         -  formats text\r\n"
                "/fi        -  indented formatting of text\r\n"
                "/h         -  list text editor commands\r\n"
                "/i# <text> -  inserts <text> before line #\r\n"
                "/l         -  lists buffer\r\n"
                "/n         -  lists buffer with line numbers\r\n"
                "/r 'a' 'b' -  replace 1st occurrence of text <a> in buffer with text <b>\r\n"
                "/ra 'a' 'b'-  replace all occurrences of text <a> within buffer with text <b>\r\n"
                "/s         -  saves text\r\n"
            );
            break;
        case 'i':
            if (!workingBuffer_.empty()) {
                return parseInsert(actions);
            } else {
                desc.sendText("Current buffer empty, nowhere to insert.\r\n");
            }
            break;
        case 'l':
            if (!workingBuffer_.empty()) {
                return parseList(actions, DisplayMode::Plain);
            } else {
                desc.sendText("Current buffer empty.\r\n");
            }
            break;
        case 'n':
            if (!workingBuffer_.empty()) {
                return parseList(actions, DisplayMode::Numbered);
            } else {
                desc.sendText("Current buffer empty.\r\n");
            }
            break;
        case 'r':
            return parseReplace(actions);
        case 's':
            return EditorResult::Save;
        default:
            desc.sendText("Invalid option.\r\n");
            break;
    }
    
    return EditorResult::Action;
}

std::vector<std::string> EditorMode::splitLines() const {
    std::vector<std::string> lines;
    boost::algorithm::iter_split(
        lines,
        workingBuffer_,
        boost::algorithm::first_finder("\r\n")
    );
    return lines;
}

std::string EditorMode::joinLines(const std::vector<std::string>& lines) const {
    return boost::algorithm::join(lines, "\r\n");
}

EditorResult EditorMode::parseDelete(const std::string& args) {
    int line_low = 0, line_high = 0;
    int parsed = sscanf(args.c_str(), " %d - %d ", &line_low, &line_high);
    
    if (parsed == 0) {
        desc.sendText("You must specify a line number or range to delete.\r\n");
        return EditorResult::Action;
    } else if (parsed == 1) {
        line_high = line_low;
    } else if (line_high < line_low) {
        desc.sendText("That range is invalid.\r\n");
        return EditorResult::Action;
    }
    
    if (line_low <= 0) {
        desc.sendText("Invalid, line numbers to delete must be higher than 0.\r\n");
        return EditorResult::Action;
    }
    
    auto lines = splitLines();
    line_low--;
    line_high--;
    
    if (line_low >= static_cast<int>(lines.size())) {
        desc.sendText("Line(s) out of range; not deleting.\r\n");
        return EditorResult::Action;
    }
    
    line_high = std::min(line_high, static_cast<int>(lines.size()) - 1);
    int deleted_count = line_high - line_low + 1;
    
    lines.erase(lines.begin() + line_low, lines.begin() + line_high + 1);
    workingBuffer_ = joinLines(lines);
    
    desc.sendFmt("{} line{} deleted.\r\n", deleted_count, (deleted_count != 1 ? "s" : ""));
    return EditorResult::Action;
}

EditorResult EditorMode::parseEdit(const std::string& args) {
    if (workingBuffer_.empty()) {
        desc.sendText("Buffer is empty, nothing to change.\r\n");
        return EditorResult::Action;
    }
    
    std::istringstream iss(args);
    std::string line_str, text;
    iss >> line_str;
    std::getline(iss, text);
    
    if (line_str.empty()) {
        desc.sendText("You must specify a line number at which to change text.\r\n");
        return EditorResult::Action;
    }
    
    int line_num = std::stoi(line_str);
    if (line_num <= 0) {
        desc.sendText("Line number must be higher than 0.\r\n");
        return EditorResult::Action;
    }
    
    if (!text.empty() && text[0] == ' ') {
        text = text.substr(1);
    }
    
    auto lines = splitLines();
    line_num--;
    
    if (line_num >= static_cast<int>(lines.size())) {
        desc.sendText("Line number out of range; change aborted.\r\n");
        return EditorResult::Action;
    }
    
    lines[line_num] = text;
    std::string new_content = joinLines(lines);
    
    if (new_content.length() > maxLength_) {
        desc.sendText("Change causes new length to exceed buffer maximum size, aborted.\r\n");
        return EditorResult::Action;
    }
    
    workingBuffer_ = new_content;
    desc.sendText("Line changed.\r\n");
    return EditorResult::Action;
}

EditorResult EditorMode::parseInsert(const std::string& args) {
    std::istringstream iss(args);
    std::string line_str, text;
    iss >> line_str;
    std::getline(iss, text);
    
    if (line_str.empty()) {
        desc.sendText("You must specify a line number before which to insert text.\r\n");
        return EditorResult::Action;
    }
    
    int line_num = std::stoi(line_str);
    if (line_num <= 0) {
        desc.sendText("Line number must be higher than 0.\r\n");
        return EditorResult::Action;
    }
    
    if (!text.empty() && text[0] == ' ') {
        text = text.substr(1);
    }
    
    auto lines = splitLines();
    line_num--;
    
    if (line_num > static_cast<int>(lines.size())) {
        desc.sendText("Line number out of range; insert aborted.\r\n");
        return EditorResult::Action;
    }
    
    std::string new_content = joinLines(lines);
    if (new_content.length() + text.length() + 3 > maxLength_) {
        desc.sendText("Insert text pushes buffer over maximum size, insert aborted.\r\n");
        return EditorResult::Action;
    }
    
    lines.insert(lines.begin() + line_num, text);
    workingBuffer_ = joinLines(lines);
    
    desc.sendText("Line inserted.\r\n");
    return EditorResult::Action;
}

EditorResult EditorMode::parseList(const std::string& args, DisplayMode mode) {
    int line_low = 1, line_high = 999999;
    
    if (!args.empty()) {
        int parsed = sscanf(args.c_str(), " %d - %d ", &line_low, &line_high);
        if (parsed == 1) {
            line_high = line_low;
        } else if (parsed == 0 || line_high < line_low) {
            desc.sendText("That range is invalid.\r\n");
            return EditorResult::Action;
        }
    }
    
    if (line_low < 1) {
        desc.sendText("Line numbers must be greater than 0.\r\n");
        return EditorResult::Action;
    }
    
    auto lines = splitLines();
    line_low--;
    line_high--;
    
    if (line_low >= static_cast<int>(lines.size())) {
        desc.sendText("Line(s) out of range; no buffer listing.\r\n");
        return EditorResult::Action;
    }
    
    line_high = std::min(line_high, static_cast<int>(lines.size()) - 1);
    
    std::string output;
    if (line_high < 999998 || line_low > 0) {
        output = fmt::format("Current buffer range [{} - {}]:\r\n", line_low + 1, line_high + 1);
    }
    
    if (mode == DisplayMode::Numbered) {
        for (int i = line_low; i <= line_high; ++i) {
            output += fmt::format("{:4d}: {}\r\n", i + 1, lines[i]);
        }
    } else {
        for (int i = line_low; i <= line_high; ++i) {
            output += lines[i] + "\r\n";
        }
    }
    
    int shown_count = line_high - line_low + 1;
    output += fmt::format("\r\n{} line{} shown.\r\n", shown_count, (shown_count != 1 ? "s" : ""));
    
    desc.sendText(output);
    return EditorResult::Action;
}

EditorResult EditorMode::parseReplace(const std::string& args) {
    if (workingBuffer_.empty()) {
        return EditorResult::Action;
    }
    
    bool rep_all = false;
    std::string working_args = args;
    
    if (!working_args.empty() && std::isalpha(working_args[0])) {
        if (working_args[0] == 'a') {
            rep_all = true;
        }
        working_args = working_args.substr(1);
    }
    
    size_t first_quote = working_args.find('\'');
    if (first_quote == std::string::npos) {
        desc.sendText("Invalid format.\r\n");
        return EditorResult::Action;
    }
    
    size_t second_quote = working_args.find('\'', first_quote + 1);
    if (second_quote == std::string::npos) {
        desc.sendText("Target string must be enclosed in single quotes.\r\n");
        return EditorResult::Action;
    }
    
    size_t third_quote = working_args.find('\'', second_quote + 1);
    if (third_quote == std::string::npos) {
        desc.sendText("No replacement string.\r\n");
        return EditorResult::Action;
    }
    
    size_t fourth_quote = working_args.find('\'', third_quote + 1);
    if (fourth_quote == std::string::npos) {
        desc.sendText("Replacement string must be enclosed in single quotes.\r\n");
        return EditorResult::Action;
    }
    
    std::string pattern = working_args.substr(first_quote + 1, second_quote - first_quote - 1);
    std::string replacement = working_args.substr(third_quote + 1, fourth_quote - third_quote - 1);
    
    size_t pattern_count = 0;
    std::string temp_str = workingBuffer_;
    size_t pos = 0;
    while ((pos = temp_str.find(pattern, pos)) != std::string::npos) {
        pattern_count++;
        pos += pattern.length();
        if (!rep_all) break;
    }
    
    if (pattern_count == 0) {
        desc.sendFmt("String '{}' not found.\r\n", pattern);
        return EditorResult::Action;
    }
    
    size_t new_length = workingBuffer_.length() - (pattern_count * pattern.length()) + (pattern_count * replacement.length());
    if (new_length > maxLength_) {
        desc.sendText("Not enough space left in buffer.\r\n");
        return EditorResult::Action;
    }
    
    if (rep_all) {
        boost::algorithm::replace_all(workingBuffer_, pattern, replacement);
    } else {
        boost::algorithm::replace_first(workingBuffer_, pattern, replacement);
    }
    
    desc.sendFmt("Replaced {} occurrence{} of '{}' with '{}'.\r\n",
               static_cast<int>(pattern_count), (pattern_count != 1 ? "s" : ""), pattern, replacement);
    return EditorResult::Action;
}

EditorResult EditorMode::parseFormat(const std::string& args) {
    desc.sendText("Text formatting not yet implemented.\r\n");
    return EditorResult::Action;
}

void EditorMode::setDisplayMode(DisplayMode mode) {
    displayMode_ = mode;
}
