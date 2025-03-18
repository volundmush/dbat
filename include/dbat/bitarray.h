#include <bitset>
#include <boost/algorithm/string.hpp>

template<size_t N>
void sprintbitarray(const std::bitset<N>& bitvector, const char *names[], int maxar, char *result) {
    *result = '\0';

    std::vector<std::string> found;

    for (size_t i = 0; i < bitvector.size(); i++) {
        if (!bitvector[i]) continue;
        found.emplace_back(names[i]);
    }

    if (found.empty())
        strcpy(result, "None ");
    else {
        auto joined = boost::algorithm::join(found, " ");
        strcpy(result, joined.c_str());
    }
}

template<typename Container>
void sprintbitarray(const Container& container, const char *names[], int maxar, char *result) {
    static_assert(std::is_enum<typename Container::value_type>::value, 
                  "Container must contain enum values");

    std::vector<std::string> found;
    for (const auto& e : container) {
        // Convert enum value to its name using magic_enum.
        found.emplace_back(std::string(magic_enum::enum_name(e)));
    }

    if (found.empty()) {
        std::strcpy(result, "None ");
    } else {
        auto joined = boost::algorithm::join(found, " ");
        std::strcpy(result, joined.c_str());
    }
}