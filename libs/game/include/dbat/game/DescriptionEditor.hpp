#pragma once
#include <memory>
#include "EditorMode.hpp"
#include "dbat/game/HasMudStrings.hpp"
#include "dbat/game/const/Form.hpp"

struct Character;

class DescriptionEditor : public EditorMode {
public:
    DescriptionEditor(
        descriptor_data& desc,
        size_t maxLength,
        std::shared_ptr<HasMudStrings> target);
    
    virtual void launch() override;
    virtual void save() override;
    virtual bool isValid() override;

protected:
    std::weak_ptr<HasMudStrings> target_;
};


class TransformationDescriptionEditor : public DescriptionEditor {
public:
    TransformationDescriptionEditor(
        descriptor_data& desc,
        size_t maxLength,
        std::shared_ptr<Character> target, Form form);

    virtual void launch() override;
    virtual void save() override;
    virtual bool isValid() override;

protected:
    Form form_;
};