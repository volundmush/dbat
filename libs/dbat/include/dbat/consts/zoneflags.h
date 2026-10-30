#pragma once

   /* Zone info: Used in zone_data.zone_flags */
constexpr int ZONE_CLOSED		= 0;
constexpr int ZONE_NOIMMORT		= 1;
constexpr int ZONE_QUEST		= 2;
constexpr int ZONE_DBALLS		= 3;
constexpr int ZONE_SPARE2		= 4;
constexpr int ZONE_SPARE3		= 5;
constexpr int ZONE_SPARE4		= 6;
constexpr int ZONE_SPARE5		= 7;
constexpr int ZONE_SPARE6		= 8;
constexpr int ZONE_SPARE7		= 9;
constexpr int ZONE_SPARE8		= 10;
constexpr int ZONE_SPARE9		= 11;
constexpr int ZONE_SPARE10		= 12;
constexpr int ZONE_SPARE11		= 13;
constexpr int ZONE_SPARE12		= 14;
constexpr int ZONE_SPARE13		= 15;
constexpr int ZONE_SPARE14		= 16;
constexpr int ZONE_SPARE15		= 17;
constexpr int ZONE_SPARE16		= 18;
constexpr int ZONE_SPARE17		= 19;
constexpr int ZONE_SPARE18		= 20;
constexpr int ZONE_SPARE19		= 21;
constexpr int ZONE_SPARE20		= 22;
constexpr int ZONE_SPARE21		= 23;
constexpr int ZONE_SPARE22		= 24;
constexpr int ZONE_SPARE23		= 25;
constexpr int ZONE_SPARE24		= 26;
constexpr int ZONE_SPARE25		= 27;
constexpr int ZONE_SPARE26		= 28;
constexpr int ZONE_SPARE27		= 29;
constexpr int ZONE_SPARE28		= 30;
constexpr int ZONE_SPARE29		= 31;
constexpr int ZONE_SPARE30		= 32;
constexpr int ZONE_SPARE31		= 33;
constexpr int ZONE_SPARE32		= 34;
constexpr int ZONE_SPARE33		= 35;

constexpr int NUM_ZONE_FLAGS          = 36;
constexpr int ZF_ARRAY_MAX	= 4;

extern const char *zone_bits[NUM_ZONE_FLAGS+1];