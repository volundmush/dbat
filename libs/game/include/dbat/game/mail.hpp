#pragma once
#include <string>
#include "Command.hpp"

constexpr int MIN_MAIL_LEVEL = 3;

int count_unreceived_mail(const std::string& playerId);
int count_unread_mail(const std::string& playerId);

void read_mail_message(Character *ch, Character *mailman, int messageNum);
