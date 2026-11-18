#pragma once
/*
 * mob_stats() is game logic the mob loader calls, until it moves to the mobs' code (F9).
 */
struct GameWorld;
struct char_data;

/* An enhanced (E) mob's abilities, rolled at random by its level and race as it loads */
void mob_stats(GameWorld *g, char_data *mob);

