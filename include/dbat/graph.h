//
// Created by volund on 10/20/21.
//
#pragma once

#include "structs.h"

// functions
extern int find_first_step(Location &src, Location &target);


std::unordered_map<Coordinates, Destination> gatherSurroundings(const Location& loc, Character *ch, int minX = -4, int maxX = 4, int minY = -4, int maxY = 4, const std::function<bool(const Destination&, Character*)> is_valid = {});