#pragma once
#include <string_view>
#include <memory>

struct HasDgScripts;

bool isUID(std::string_view uid);
std::shared_ptr<HasDgScripts> resolveUID(std::string_view uid);