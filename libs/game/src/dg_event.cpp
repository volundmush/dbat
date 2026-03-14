#include "dbat/game/DgScript.hpp"
#include "dbat/game/dg_event.hpp"
#include "dbat/game/dg_scripts.hpp"
#include "dbat/util/FilterWeak.hpp"

/* Process any events whose time has come. */
void event_process(uint64_t heart_pulse, double deltaTime) {
    // copy the queue to avoid any issues with the queue being modified while we're processing it
    auto waiting = triggerSubscriptions.all("waiting");

    for (auto trig : dbat::util::filter_raw(waiting)) {
        trig->waiting = std::max<double>(0.0, trig->waiting - deltaTime);
        if(trig->waiting == 0.0) {
            triggerSubscriptions.unsubscribe("waiting", trig);
            triggerSubscriptions.subscribe("queued", trig);
        }
    }

    int toProcess = 5;
    auto queued = triggerSubscriptions.all("queued");
    for(auto trig : dbat::util::filter_raw(queued)) {
        if(toProcess <= 0) break;
        triggerSubscriptions.unsubscribe("queued", trig);
        trig->execute();
        toProcess--;
    }
}
