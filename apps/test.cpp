//
// Created by volund on 11/14/21.
//


#include "sysdep.h"
#include "sqlitepp.h"
#include <iostream>

int main(int argc, char **argv) {
    sqlitepp::db d("test.sqlite3");
    assert(d.is_open());
    std::cout << "Yup it works!" << std::endl;
}
