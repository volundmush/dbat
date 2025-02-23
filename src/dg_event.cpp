#include "dbat/dg_event.h"
#include "dbat/dg_scripts.h"


/* Process any events whose time has come. */
void event_process(uint64_t heart_pulse, double deltaTime) {
    // copy the queue to avoid any issues with the queue being modified while we're processing it
    auto waiting = triggerSubscriptions.all("waiting");

    for (auto trig : filter_raw(waiting)) {
        trig->waiting = std::max<double>(0.0, trig->waiting - deltaTime);
        if(trig->waiting == 0.0) {
            triggerSubscriptions.unsubscribe("waiting", trig);
            triggerSubscriptions.subscribe("queued", trig);
        }
    }

    int toProcess = 5;
    auto queued = triggerSubscriptions.all("queued");
    for(auto trig : filter_raw(queued)) {
        if(toProcess <= 0) break;
        triggerSubscriptions.unsubscribe("queued", trig);
        
        script_driver(trig->owner.get(), trig, trig->attach_type, TRIG_RESTART);
        toProcess--;
    }
}
