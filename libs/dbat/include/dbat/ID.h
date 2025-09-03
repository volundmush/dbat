#pragma once
#include <cstdint>

extern int64_t lastCharacterID;
extern int64_t lastObjectID;
extern int64_t lastAccountID;
extern int64_t lastStructureID;
extern int64_t lastAreaID;
extern int64_t lastGridTemplateID;

extern int lastRoomID;
extern int lastZoneID;
extern int lastShopID;
extern int lastGuildID;
extern int lastScriptID;

template <class Counter, class Container>
Counter getNextID(Counter& counter, const Container& cont) {
    static_assert(std::is_arithmetic_v<Counter>,
                  "Counter must be a numeric type");
    while(cont.contains(counter)) counter++;
    return counter;
};