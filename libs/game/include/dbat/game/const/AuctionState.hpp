#pragma once

/* AUCTIONING STATES */
constexpr int AUC_NULL_STATE = 0;  /* not doing anything */
constexpr int AUC_OFFERING = 1;    /* object has been offfered */
constexpr int AUC_GOING_ONCE = 2;  /* object is going once! */
constexpr int AUC_GOING_TWICE = 3; /* object is going twice! */
constexpr int AUC_LAST_CALL = 4;   /* last call for the object! */
constexpr int AUC_SOLD = 5;
/* AUCTION CANCEL STATES */
constexpr int AUC_NORMAL_CANCEL = 6; /* normal cancellation of auction */
constexpr int AUC_QUIT_CANCEL = 7;   /* auction canclled because player quit */
constexpr int AUC_WIZ_CANCEL = 8;    /* auction cancelled by a god */
/* OTHER JUNK */
constexpr int AUC_STAT = 9;
constexpr int AUC_BID = 10;