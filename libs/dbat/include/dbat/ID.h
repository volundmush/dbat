#pragma once
#include <cstdint>
#include <type_traits>


extern int64_t lastAccountID;
extern int64_t lastStructureID;
extern int64_t lastAreaID;
extern int64_t lastGridTemplateID;

extern int lastZoneID;
extern int lastShopID;
extern int lastGuildID;
extern int lastScriptID;

template <class Counter, class Container>
requires std::is_arithmetic<Counter>::value && requires (Container c, Counter i) { c.contains(i); }
Counter getNextID(Counter& counter, const Container& cont) {
    while(cont.contains(counter)) counter++;
    return counter;
};