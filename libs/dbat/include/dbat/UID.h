#pragma once
#include <string>
#include <memory>

struct HasDgScripts;

bool isUID(const std::string &uid);
std::shared_ptr<HasDgScripts> resolveUID(const std::string &uid);