#pragma once
#include <string_view>

struct descriptor_data;

class DescriptorMode {
    public:
        DescriptorMode(descriptor_data& desc) : desc(desc) {}
        virtual ~DescriptorMode() = default;

        virtual void handleInput(std::string_view input);
        virtual void onLaunch();
        virtual void onClose();
        virtual void onReplace();
        virtual bool isModeValid();
    protected:
        descriptor_data& desc;
};