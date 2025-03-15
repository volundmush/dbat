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