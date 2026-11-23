#pragma once
#include <string_view>

#include "dbat/consts/types.hpp"
#include "dbat/descriptor.hpp"

int desc_get_connected(const struct descriptor_data *d);
void desc_set_connected(struct descriptor_data *d, int v);

bool desc_is_playing(const struct descriptor_data *d);

/*
 * A connection as the tests (and anything outside the networking code) see it: its output waiting to be sent, and its
 * socket by number. What holds them is the networking code's own business.
 */
/* The output queued for d and not yet sent */
std::string_view desc_output(const struct descriptor_data *d);
/* Throws away d's queued output, leaving it an empty buffer (also how a bare descriptor gets one) */
void desc_discard_output(GameWorld *g, struct descriptor_data *d);
/* Replaces d's queued output with text, which may be no longer */
void desc_rewrite_output(struct descriptor_data *d, std::string_view text);
/* The socket d reads and writes, as the system numbers it */
net::native_handle desc_socket(const struct descriptor_data *d);
/* A new connection on socket, from host, in the game's list: logging in, with nothing yet sent or typed */
struct descriptor_data *add_descriptor(GameWorld *g, net::native_handle socket, std::string_view host);

void customCreate(struct descriptor_data *d);
void customRead(GameWorld *g, struct descriptor_data *d, int type, std::string_view name);
void customWrite(struct char_data *ch, struct obj_data *obj);

