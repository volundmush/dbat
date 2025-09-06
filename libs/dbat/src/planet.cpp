#include "dbat/Character.h"
#include "dbat/Room.h"
#include "dbat/planet.h"
//#include "dbat/db.h"
#include "dbat/comm.h"
//#include "dbat/send.h"


void displayLandSpots(Character *ch, std::string_view planet_name, const std::map<std::string, Location>& locations) {
    const int line_length = 60;  // Adjust the max line length as needed
    int current_length = 0;

    ch->sendFmt("@D------------------[ {}@D ]------------------@c\n", planet_name);

    int i = 0;
    for (const auto& [location, vnum] : locations) {
        // Check if adding this location would exceed the line length
        if (current_length + location.length() + 2 > line_length) { // +2 for the ", "
            ch->sendText("\n");  // Start a new line
            current_length = 0;
        }

        // Add the location to the line
                ch->send_to("%s", location.c_str());
        current_length += location.length();

        // Add a comma and space unless it's the last item
        if (i < locations.size() - 1) {
                        ch->sendText(", ");
            current_length += 2;
        }
        i++;
    }

        ch->sendText(".\n");  // End the list with a period and new line
        ch->sendText("@D---------------------------------------------@n\n");
}
