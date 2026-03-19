#pragma once
#include <string_view>
#include <expected>

std::expected<int, std::string> search_help(std::string_view argument, int level);
