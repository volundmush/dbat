#pragma once
#include <fmt/format.h>
#include <fmt/printf.h>
#include "Log.hpp"
#include "Typedefs.hpp"
#include "const/WhereFlag.hpp"

void send_to_all_impl(const std::string& message);

template<typename... Args>
void send_to_all(fmt::string_view format, Args&&... args) {
    try {
        std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
        if(formatted_string.empty()) return;

        send_to_all_impl(formatted_string);
    }
    catch(const std::exception &e) {
        LERROR("SYSERR: Format error in send_to_all: %s", e.what());
        LERROR("Template was: %s", format.data());
    }
}

void send_to_outdoor_impl(const std::string& message);

template<typename... Args>
void send_to_outdoor(fmt::string_view format, Args&&... args) {
    try {
        std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
        if(formatted_string.empty()) return;

        send_to_outdoor_impl(formatted_string);
    }
    catch(const std::exception &e) {
        LERROR("SYSERR: Format error in send_to_outdoor: %s", e.what());
        LERROR("Template was: %s", format.data());
    }
}

void send_to_moon_impl(const std::string& message);

template<typename... Args>
void send_to_moon(fmt::string_view format, Args&&... args) {
    try {
        std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
        if(formatted_string.empty()) return;

        send_to_moon_impl(formatted_string);
    }
    catch(const std::exception &e) {
        LERROR("SYSERR: Format error in send_to_moon: %s", e.what());
        LERROR("Template was: %s", format.data());
    }
}

void send_to_planet_impl(int type, WhereFlag planet, const std::string& message);

template<typename... Args>
void send_to_planet(int type, WhereFlag planet, fmt::string_view format, Args&&... args) {
    try {
        std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
        if(formatted_string.empty()) return;
        send_to_planet_impl(type, planet, formatted_string);
    }
    catch(const std::exception &e) {
        LERROR("SYSERR: Format error in send_to_planet: %s", e.what());
        LERROR("Template was: %s", format.data());
    }
}

void send_to_range_impl(room_vnum start, room_vnum finish, const std::string& message);

template<typename... Args>
void send_to_range(room_vnum start, room_vnum finish, fmt::string_view format, Args&&... args) {
    if (start > finish) {
        LERROR("send_to_range passed start room value greater then finish.");
        return;
    }

    try {
        std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
        if(formatted_string.empty()) return;

        send_to_range_impl(start, finish, formatted_string);
    }
    catch(const std::exception &e) {
        LERROR("SYSERR: Format error in send_to_imm: %s", e.what());
        LERROR("Template was: %s", format.data());
    }
}

void send_to_imm_impl(const std::string& message);

template<typename... Args>
void send_to_imm(fmt::string_view format, Args&&... args) {
    try {
        std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
        if(formatted_string.empty()) return;

        send_to_imm_impl(formatted_string);
    }
    catch(const std::exception &e) {
        LERROR("SYSERR: Format error in send_to_imm: %s", e.what());
        LERROR("Template was: %s", format.data());
    }
}

void game_info_impl(const std::string& message);

template<typename... Args>
void game_info(fmt::string_view format, Args&&... args) {
    try {
        std::string formatted_string = fmt::sprintf(format, std::forward<Args>(args)...);
        if(formatted_string.empty()) return;

        game_info_impl(formatted_string);
    }
    catch(const std::exception &e) {
        LERROR("SYSERR: Format error in game_info: %s", e.what());
        LERROR("Template was: %s", format.data());
    }

}
