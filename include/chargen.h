#pragma once
#include "net.h"
#include "structs.h"
#include "races.h"
#include "class.h"

namespace net {

    class ChargenParser : public ConnectionParser {
    public:
        ChargenParser(struct connection_data *conn, const std::string &na);
        void parse(const std::string &txt) override;
        void start() override;

    protected:
        char_data *ch;
        int state;
        int total;
        int ccpoints;
        int negcount;
        int roll_stats(int type, int bonus);
        void display_bonus_menu(int type);
        race::RaceMap valid_races();
        void display_races();
        void display_races_sub();
        sensei::SenseiMap valid_classes();
        void display_classes_sub();
        void display_classes();
        void display_races_help();
        void display_classes_help();
        void exchange_ccpoints(int value);
        int opp_bonus(int value, int type);
        bool bonus_exclusive(int type, int value, int exc);
    };

}