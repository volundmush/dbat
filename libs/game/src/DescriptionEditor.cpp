#include "dbat/game/DescriptionEditor.hpp"
#include "dbat/game/Descriptor.hpp"
#include "dbat/game/CharacterUtils.hpp"



DescriptionEditor::DescriptionEditor(descriptor_data& desc, size_t maxLength, std::shared_ptr<HasMudStrings> target)
 : EditorMode(desc, maxLength), target_(target) {
 }
    

void DescriptionEditor::launch() {
    if(auto target = target_.lock()) {
        workingBuffer_ = target->look_description;
    }
}


void DescriptionEditor::save() {
    if(auto target = target_.lock()) {
        target->look_description = workingBuffer_;
    }
}

bool DescriptionEditor::isValid() {
    if (auto target = target_.lock()) {
        return true;
    }
    desc.sendText("The target of your description edit no longer exists. Edit aborted.");
    return false;
}


TransformationDescriptionEditor::TransformationDescriptionEditor(descriptor_data& desc, size_t maxLength, std::shared_ptr<Character> target, Form form)
    : DescriptionEditor(desc, maxLength, target), form_(form) {
}

void TransformationDescriptionEditor::launch() {
    if(auto tar = target_.lock()) {
        auto t = std::dynamic_pointer_cast<Character>(tar);
        if(auto find = t->transforms.find(form_); find != t->transforms.end()) {
            workingBuffer_ = find->second.description;
        }
    }
}

void TransformationDescriptionEditor::save() {
    if(auto tar = target_.lock()) {
        auto t = std::dynamic_pointer_cast<Character>(tar);
        if(auto find = t->transforms.find(form_); find != t->transforms.end()) {
            find->second.description = workingBuffer_;
        }
    }
}


bool TransformationDescriptionEditor::isValid() {
    if (auto tar = target_.lock()) {
        auto t = std::dynamic_pointer_cast<Character>(tar);
        if(t->transforms.find(form_) != t->transforms.end()) {
            return true;
        }
    }
    desc.sendText("The target of your description edit no longer exists. Edit aborted.");
    return false;
}