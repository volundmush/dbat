//
// Created by volund on 10/20/21.
//
#pragma once
struct Object;
struct Character;
struct Structure;

struct VehicleInUse {
    Object* control{};
    Structure* vehicle{};
};

// functions
void handle_drive_direction(Character *ch, Structure *vehicle, int dir, int speed);

Object *find_control(Character *ch);

Structure *find_vehicle_by_vnum(int vnum);

Structure *find_hatch_by_vnum(int vnum);


// commands

