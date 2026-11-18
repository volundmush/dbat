#pragma once
/*
 * The two files the game once kept as raw struct dumps, etc/hcontrol and etc/plrmail, read as the Linux x86-64 build
 * wrote them, so that a deployment still holding them converts them to text on its first boot. This is the only
 * code that knows those layouts; it can go once every deployment has converted.
 */
#include <string_view>
#include <vector>

#include "dbat/house.hpp"
#include "dbat/mail.hpp"

namespace legacy {

/* A house_control_rec as written: 200 bytes, little-endian, with the padding */
constexpr size_t house_record_size = 200;
/* A mail block: 256 bytes */
constexpr size_t mail_block_size = 256;

/* Whether text is an old binary file of records of record_size: it lacks the text format's header line, and is a
   whole number of records (an empty file is one with none) */
bool is_binary(std::string_view text, std::string_view header, size_t record_size);

/* The house records of an old hcontrol file, all of them, as the old reader read them before checking any */
std::vector<house_control_rec> houses(std::string_view bytes);

/* The letters of an old plrmail file, in the order the old reader indexed them (by their header blocks' place in
   the file), each with its text gathered from its blocks */
std::vector<mail_letter> letters(std::string_view bytes);

} // namespace legacy
