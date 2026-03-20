#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <memory>
#include <vector>

#include "DescriptorMode.hpp"

struct descriptor_data;

enum class EditorResult {
    Continue,
    Action,
    Save,
    Abort
};

enum class DisplayMode {
    Plain,
    Numbered,
    Formatted
};

class EditorMode : public DescriptorMode {
public:
    EditorMode(descriptor_data& desc, size_t maxLength);
    
    void handleInput(std::string_view input) override;
    void onLaunch() override;
    void onClose() override;
    void onReplace() override;
    bool isModeValid() override;
    
    void setDisplayMode(DisplayMode mode);
    
protected:
    size_t maxLength_;
    std::string workingBuffer_;
    DisplayMode displayMode_ = DisplayMode::Plain;
    
    virtual void launch();
    virtual void cleanup();
    virtual bool validate();
    virtual void save();
    virtual void abort();
    virtual void displayBuffer();
    virtual bool isValid();
    
    EditorResult processCommand(std::string_view input);
    
    std::vector<std::string> splitLines() const;
    std::string joinLines(const std::vector<std::string>& lines) const;
    
    EditorResult parseDelete(const std::string& args);
    EditorResult parseEdit(const std::string& args);
    EditorResult parseInsert(const std::string& args);
    EditorResult parseList(const std::string& args, DisplayMode mode);
    EditorResult parseReplace(const std::string& args);
    EditorResult parseFormat(const std::string& args);
};
