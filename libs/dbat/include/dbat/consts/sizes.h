#pragma once
/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
constexpr int SIZE_UNDEFINED	= (-1);
constexpr int SIZE_FINE	= 0;
constexpr int SIZE_DIMINUTIVE	= 1;
constexpr int SIZE_TINY	= 2;
constexpr int SIZE_SMALL	= 3;
constexpr int SIZE_MEDIUM	= 4;
constexpr int SIZE_LARGE	= 5;
constexpr int SIZE_HUGE	= 6;
constexpr int SIZE_GARGANTUAN	= 7;
constexpr int SIZE_COLOSSAL	= 8;

constexpr int NUM_SIZES         = 9;

extern const char *size_names[NUM_SIZES+1];