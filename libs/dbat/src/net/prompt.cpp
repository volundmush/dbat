/*************************************************************************
*   File: prompt.cpp                                    Part of CircleMUD *
*  Usage: The prompt shown after output                                  *
*         (formerly part of comm.c)                                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "dbat/iterate.hpp"
#include "dbat/consts/songs.hpp"
#include "dbat/utils.hpp"

#include "comm_internal.hpp"

std::string make_prompt(GameWorld *g, struct descriptor_data *d)
{
  std::string prompt;
  struct obj_data *chair = nullptr;
  int flagged            = false;

  /* Note, prompt is truncated at MAX_PROMPT_LENGTH - 1 chars, at the end. */

  if (d->str) {
    if (desc_get_connected(d) == CON_EXDESC) {
      prompt = "Enter Description(/h for editor help)> ";
    } else if (char_plr_flagged(d->character, PLR_WRITING) && !char_plr_flagged(d->character, PLR_MAILING)) {
      prompt = "Enter Message(/h for editor help)> ";
    } else if (char_plr_flagged(d->character, PLR_MAILING)) {
      prompt = "Enter Mail Message(/h for editor help)> ";
    } else {
      prompt = "Enter Message> ";
    }
  } else if (desc_get_connected(d) == CON_PLAYING && !char_is_npc(d->character)) {
    if (char_get_invis_level(g, d->character)) {
      prompt += str_format("i{:d} ", char_get_invis_level(g, d->character));
    }
    /* show only when below 25% */
    if (char_prf_flagged(g, d->character, PRF_DISPAUTO) && char_get_level(d->character) >= 500) {
      struct char_data *ch = d->character;
      if (getCurPL(g, ch) << 2 < getEffMaxPL(g, ch)) {
        prompt += str_format("PL: {:d} ", getCurPL(g, ch));
      }
      if ((getCurST(g, ch)) << 2 < getMaxST(g, ch)) {
        prompt += str_format("STA: {:d} ", (getCurST(g, ch)));
      }
      if (char_get_ki(ch) << 2 < char_get_max_ki(ch)) {
        prompt += str_format("KI: {:d} ", char_get_ki(ch));
      }
    } else { /* not auto prompt */
      prompt += "@w";
      if (char_plr_flagged(d->character, PLR_SELFD)) {
        prompt += str_format("@D[@RSELF-D@r: @w{}@D]@n", char_plr_flagged(d->character, PLR_SELFD2) ? "READY" : "PREP");
        flagged = true;
      }
      if (char_is_halfbreed(d->character) && !char_plr_flagged(d->character, PLR_FURY) && char_prf_flagged(g, d->character, PRF_FURY)) {
        prompt += str_format("@D[@mFury@W: @r{:d}@D]@w", char_get_fury(d->character));
        flagged = true;
      }
      if (char_is_halfbreed(d->character) && char_plr_flagged(d->character, PLR_FURY) && char_prf_flagged(g, d->character, PRF_FURY)) {
        prompt += "@D[@mFury@W: @rENGAGED@D]@w";
        flagged = true;
      }
      if (has_mail(g, char_get_idnum(d->character)) && !char_prf_flagged(g, d->character, PRF_NMWARN) && (char_get_admlevel(d->character) > 0)) {
        prompt += "CHECK MAIL - ";
        flagged = true;
      }
      if (char_get_kaioken(d->character) > 0 && char_get_admlevel(d->character) > 0) {
        prompt += str_format("KAIOKEN X{:d} - ", char_get_kaioken(d->character));
        flagged = true;
      }
      if (char_get_powerattack(d->character) > 0) {
        prompt += str_format("{} - ", song_types[char_get_powerattack(d->character)]);
        flagged = true;
      }
      if (char_get_kaioken(d->character) > 0 && char_get_admlevel(d->character) <= 0) {
        prompt += str_format("KAIOKEN X{:d} - ", char_get_kaioken(d->character));
        flagged = true;
      }
      if (has_mail(g, char_get_idnum(d->character)) && (char_get_admlevel(d->character) <= 0) && !char_prf_flagged(g, d->character, PRF_NMWARN)) {
        prompt += "CHECK MAIL - ";
        flagged = true;
      }
      if (d->snooping && d->snooping->character != nullptr) {
        prompt += str_format("Snooping: ({}) - ", char_get_name(d->snooping->character));
        flagged = true;
      }
      if (char_get_drag(g, d->character) && char_get_drag(g, d->character) != nullptr) {
        prompt += str_format("Dragging: ({}) - ", char_get_name(char_get_drag(g, d->character)));
        flagged = true;
      }
      if (char_prf_flagged(g, d->character, PRF_BUILDWALK)) {
        prompt += "BUILDWALKING - ";
        flagged = true;
      }
      if (char_aff_flagged(d->character, AFF_FLYING) && !char_prf_flagged(g, d->character, PRF_NODEC)) {
        prompt += "FLYING - ";
        flagged = true;
      }
      if (char_aff_flagged(d->character, AFF_HIDE) && !char_prf_flagged(g, d->character, PRF_NODEC)) {
        prompt += "HIDING - ";
        flagged = true;
      }
      if (char_plr_flagged(d->character, PLR_SPAR) && !char_prf_flagged(g, d->character, PRF_NODEC)) {
        prompt += "SPARRING - ";
        flagged = true;
      }
      if (char_plr_flagged(d->character, PLR_NOSHOUT)) {
        prompt += "MUTED - ";
        flagged = true;
      }
      if (char_get_combo(d->character) == 51) {
        prompt += "Combo (Bash) - ";
        flagged = true;
      }
      if (char_get_combo(d->character) == 52) {
        prompt += "Combo (Headbutt) - ";
        flagged = true;
      }
      if (char_get_combo(d->character) == 56) {
        prompt += "Combo (Tailwhip) - ";
        flagged = true;
      }
      if (char_get_combo(d->character) == 0) {
        prompt += "Combo (Punch) - ";
        flagged = true;
      }
      if (char_get_combo(d->character) == 1) {
        prompt += "Combo (Kick) - ";
        flagged = true;
      }
      if (char_get_combo(d->character) == 2) {
        prompt += "Combo (Elbow) - ";
        flagged = true;
      }
      if (char_get_combo(d->character) == 3) {
        prompt += "Combo (Knee) - ";
        flagged = true;
      }
      if (char_get_combo(d->character) == 4) {
        prompt += "Combo (Roundhouse) - ";
        flagged = true;
      }
      if (char_get_combo(d->character) == 5) {
        prompt += "Combo (Uppercut) - ";
        flagged = true;
      }
      if (char_get_combo(d->character) == 6) {
        prompt += "Combo (Slam) - ";
        flagged = true;
      }
      if (char_get_combo(d->character) == 8) {
        prompt += "Combo (Heeldrop) - ";
        flagged = true;
      }
      if (char_prf_flagged(g, d->character, PRF_AFK) && !char_prf_flagged(g, d->character, PRF_NODEC)) {
        prompt += "AFK - ";
        flagged = true;
      }
      if (char_plr_flagged(d->character, PLR_FISHING) && !char_prf_flagged(g, d->character, PRF_NODEC)) {
        prompt += "FISHING -";
        flagged = true;
      }
      if (flagged == true) {
        prompt += "@n\n";
      }
      if ((char_get_sits(g, d->character) && char_plr_flagged(d->character, PLR_HEALT)) && !char_prf_flagged(g, d->character, PRF_NODEC)) {
        chair   = char_get_sits(g, d->character);
        prompt += "@c<@CFloating inside a healing tank@c>@n\r\n";
        flagged = true;
      }
      if ((char_get_sits(g, d->character) && char_get_position(d->character) == POS_SITTING) && !char_prf_flagged(g, d->character, PRF_NODEC)) {
        chair   = char_get_sits(g, d->character);
        prompt += str_format("Sitting on: {}\r\n", obj_get_short_description(chair));
        flagged = true;
      }
      if ((char_get_sits(g, d->character) && char_get_position(d->character) == POS_RESTING) && !char_prf_flagged(g, d->character, PRF_NODEC)) {
        chair   = char_get_sits(g, d->character);
        prompt += str_format("Resting on: {}\r\n", obj_get_short_description(chair));
        flagged = true;
      }
      if ((char_get_sits(g, d->character) && char_get_position(d->character) == POS_SLEEPING) && !char_prf_flagged(g, d->character, PRF_NODEC)) {
        chair   = char_get_sits(g, d->character);
        prompt += str_format("Sleeping on: {}\r\n", obj_get_short_description(chair));
        flagged = true;
      }
      if (char_aff_flagged(d->character, AFF_POSITION) && !char_prf_flagged(g, d->character, PRF_NODEC)) {
        chair   = char_get_sits(g, d->character);
        prompt += "(Best Position)\r\n";
        flagged = true;
      }
      if (char_get_charge(d->character) < getMaxKI(g, d->character) * .01 && char_get_charge(d->character) > 0) {
        char_set_charge(d->character, 0);
      }
      if (char_get_charge(d->character) > 0) {
        int64_t charge = char_get_charge(d->character);
        if (!char_prf_flagged(g, d->character, PRF_NODEC) && !char_prf_flagged(g, d->character, PRF_DISPERC)) {
          if (charge >= getMaxKI(g, d->character)) {
            prompt += "@CCharge @D[@G==@D<@RMAX@D>@G===@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .95) {
            prompt += "@CCharge @D[@G=========-@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .9) {
            prompt += "@CCharge @D[@G=========@g-@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .85) {
            prompt += "@CCharge @D[@G========-@g-@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .80) {
            prompt += "@CCharge @D[@G========@g--@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .75) {
            prompt += "@CCharge @D[@G=======-@g--@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .70) {
            prompt += "@CCharge @D[@G=======@g---@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .65) {
            prompt += "@CCharge @D[@G======-@g---@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .60) {
            prompt += "@CCharge @D[@G======@g----@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .55) {
            prompt += "@CCharge @D[@G=====-@g----@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .50) {
            prompt += "@CCharge @D[@G=====@g-----@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .45) {
            prompt += "@CCharge @D[@G====-@g-----@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .40) {
            prompt += "@CCharge @D[@G====@g------@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .35) {
            prompt += "@CCharge @D[@G===-@g------@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .30) {
            prompt += "@CCharge @D[@G===@g-------@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .25) {
            prompt += "@CCharge @D[@G==-@g-------@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .20) {
            prompt += "@CCharge @D[@G==@g--------@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .15) {
            prompt += "@CCharge @D[@G=-@g--------@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .10) {
            prompt += "@CCharge @D[@G=@g---------@D]@n\n";
          } else if (charge >= getMaxKI(g, d->character) * .05) {
            prompt += "@CCharge @D[@G-@g---------@D]@n\n";
          } else {
            prompt += "@CCharge @D[@g----------@D]@n\n";
          }
        }
        if (char_prf_flagged(g, d->character, PRF_DISPERC) && !char_prf_flagged(g, d->character, PRF_NODEC)) {
          if (char_get_charge(d->character) > 0) {
            int64_t perc = (char_get_charge(d->character) * 100) / getMaxKI(g, d->character);
            prompt += str_format("@D[@BCharge@Y: @C{:d}{}@D]@n\n", perc, "%");
          }
        }
        if (char_prf_flagged(g, d->character, PRF_NODEC)) {
          if (charge > 0) {
            int64_t perc = (charge * 100) / getMaxKI(g, d->character);
            prompt += str_format("Ki is charged to {:d} percent.\n", perc);
          }
        }
      }
      if (char_aff_flagged(d->character, AFF_FIRESHIELD)) {
        prompt += "@D(@rF@RI@YR@rE@RS@YH@rI@RE@YL@rD@D)@n\n";
      }
      if (char_aff_flagged(d->character, AFF_SANCTUARY)) {
        if (char_prf_flagged(g, d->character, PRF_DISPERC) && !char_prf_flagged(g, d->character, PRF_NODEC)) {
          if (char_get_barrier(d->character) > 0) {
            int64_t perc = (char_get_barrier(d->character) * 100) / getMaxKI(g, d->character);
            prompt += str_format("@D[@GBarrier@Y: @B{:d}{}@D]@n\n", perc, "%");
          }
        }

        if (!char_prf_flagged(g, d->character, PRF_NODEC) && !char_prf_flagged(g, d->character, PRF_DISPERC)) {
          if (char_get_barrier(d->character) >= getMaxKI(g, d->character) * .75) {
            prompt += "@BBarrier @D[@C==MAX==@D]@n\n";
          } else if (char_get_barrier(d->character) >= getMaxKI(g, d->character) * .70) {
            prompt += "@BBarrier @D[@C=======@D]@n\n";
          } else if (char_get_barrier(d->character) >= getMaxKI(g, d->character) * .65) {
            prompt += "@BBarrier @D[@C======-@D]@n\n";
          } else if (char_get_barrier(d->character) >= getMaxKI(g, d->character) * .60) {
            prompt += "@BBarrier @D[@C======@c-@D]@n\n";
          } else if (char_get_barrier(d->character) >= getMaxKI(g, d->character) * .55) {
            prompt += "@BBarrier @D[@C=====-@c-@D]@n\n";
          } else if (char_get_barrier(d->character) >= getMaxKI(g, d->character) * .50) {
            prompt += "@BBarrier @D[@C=====@c--@D]@n\n";
          } else if (char_get_barrier(d->character) >= getMaxKI(g, d->character) * .45) {
            prompt += "@BBarrier @D[@C====-@c--@D]@n\n";
          } else if (char_get_barrier(d->character) >= getMaxKI(g, d->character) * .40) {
            prompt += "@BBarrier @D[@C====@c---@D]@n\n";
          } else if (char_get_barrier(d->character) >= getMaxKI(g, d->character) * .35) {
            prompt += "@BBarrier @D[@C===-@c---@D]@n\n";
          } else if (char_get_barrier(d->character) >= getMaxKI(g, d->character) * .30) {
            prompt += "@BBarrier @D[@C===@c----@D]@n\n";
          } else if (char_get_barrier(d->character) >= getMaxKI(g, d->character) * .25) {
            prompt += "@BBarrier @D[@C==-@c----@D]@n\n";
          } else if (char_get_barrier(d->character) >= getMaxKI(g, d->character) * .20) {
            prompt += "@BBarrier @D[@C==@c-----@D]@n\n";
          } else if (char_get_barrier(d->character) >= getMaxKI(g, d->character) * .15) {
            prompt += "@BBarrier @D[@C=-@c-----@D]@n\n";
          } else if (char_get_barrier(d->character) >= getMaxKI(g, d->character) * .10) {
            prompt += "@BBarrier @D[@C=@c------@D]@n\n";
          } else if (char_get_barrier(d->character) >= getMaxKI(g, d->character) * .05) {
            prompt += "@BBarrier @D[@C-@c------@D]@n\n";
          } else if (char_get_barrier(d->character) < getMaxKI(g, d->character) * .05) {
            prompt += "@BBarrier @D[@C--Low-@D]@n\n";
          }
        }
        if (char_prf_flagged(g, d->character, PRF_NODEC)) {
          if (char_get_barrier(d->character) > 0) {
            int64_t perc = (char_get_barrier(d->character) * 100) / getMaxKI(g, d->character);
            prompt += str_format("A barrier charged to {:d} percent surrounds you.@n\n", perc);
          }
        }
      }
      if (!char_prf_flagged(g, d->character, PRF_DISPERC)) {
        if (char_prf_flagged(g, d->character, PRF_DISPHP)) {
          std::string_view col      = "n";
          struct char_data *ch = d->character;
          if (getMaxPL(g, ch) > getMaxPLTrans(g, ch))
            col = "g";
          else if (isWeightedPL(g, ch))
            col = "m";
          else if (getCurHealthPercent(g, ch) > .5)
            col = "c";
          else if (getCurHealthPercent(g, ch) > .1)
            col = "y";
          else if (getCurHealthPercent(g, ch) <= .1)
            col = "r";

          prompt += str_format("@D[@RPL@n@Y: @{}{}@D]@n", col, add_commas(getCurPL(g, ch)));
        }
      } else if (char_prf_flagged(g, d->character, PRF_DISPHP)) {

        struct char_data *ch = d->character;
        double perc          = ((double)getCurHealth(g, ch) / (double)getMaxPLTrans(g, ch)) * 100;
        std::string_view col      = "n";
        if (perc > 100)
          col = "g";
        else if (perc >= 70)
          col = "c";
        else if (perc >= 51)
          col = "Y";
        else if (perc >= 20)
          col = "y";
        else
          col = "r";

        prompt += str_format("@D[@RPL@n@Y: @{}{:d}{}@D]@n", col, (int)perc, "@w%");
      }
      if (!char_prf_flagged(g, d->character, PRF_DISPERC)) {
        if (char_prf_flagged(g, d->character, PRF_DISPKI) &&
            (getCurKI(g, d->character)) > getMaxKI(g, d->character) / 2) {
          prompt += str_format("@D[@CKI@Y: @c{}@D]@n", add_commas((getCurKI(g, d->character))));
        } else if (char_prf_flagged(g, d->character, PRF_DISPKI) &&
                   (getCurKI(g, d->character)) > getMaxKI(g, d->character) / 10) {
          prompt += str_format("@D[@CKI@Y: @y{}@D]@n", add_commas((getCurKI(g, d->character))));
        } else if (char_prf_flagged(g, d->character, PRF_DISPKI) &&
                   (getCurKI(g, d->character)) <= getMaxKI(g, d->character) / 10) {
          prompt += str_format("@D[@CKI@Y: @r{}@D]@n", add_commas((getCurKI(g, d->character))));
        }
      } else if (char_prf_flagged(g, d->character, PRF_DISPKI)) {
        int64_t power = (getCurKI(g, d->character)), maxpower = getMaxKI(g, d->character);
        int perc = 0;
        if (power <= 0) {
          power = 1;
        }
        if (maxpower <= 0) {
          maxpower = 1;
        }
        perc = (power * 100) / maxpower;
        if (perc > 100) {
          prompt += str_format("@D[@CKI@n@Y: @G{:d}{}@D]@n", perc, "@w%");
        } else if (perc >= 70) {
          prompt += str_format("@D[@CKI@n@Y: @c{:d}{}@D]@n", perc, "@w%");
        } else if (perc >= 51) {
          prompt += str_format("@D[@CKI@n@Y: @Y{:d}{}@D]@n", perc, "@w%");
        } else if (perc >= 20) {
          prompt += str_format("@D[@CKI@n@Y: @y{:d}{}@D]@n", perc, "@w%");
        } else {
          prompt += str_format("@D[@CKI@n@Y: @r{:d}{}@D]@n", perc, "@w%");
        }
      }
      if (!char_prf_flagged(g, d->character, PRF_DISPERC)) {
        if (char_prf_flagged(g, d->character, PRF_DISPMOVE) &&
            (getCurST(g, d->character)) > getMaxST(g, d->character) / 2) {
          prompt += str_format("@D[@GSTA@Y: @c{}@D]@n", add_commas((getCurST(g, d->character))));
        } else if (char_prf_flagged(g, d->character, PRF_DISPMOVE) &&
                   (getCurST(g, d->character)) > getMaxST(g, d->character) / 10) {
          prompt += str_format("@D[@GSTA@Y: @y{}@D]@n", add_commas((getCurST(g, d->character))));
        } else if (char_prf_flagged(g, d->character, PRF_DISPMOVE) &&
                   (getCurST(g, d->character)) <= getMaxST(g, d->character) / 10) {
          prompt += str_format("@D[@GSTA@Y: @r{}@D]@n", add_commas((getCurST(g, d->character))));
        }
      } else if (char_prf_flagged(g, d->character, PRF_DISPMOVE)) {
        int64_t power = (getCurST(g, d->character)), maxpower = getMaxST(g, d->character);
        int perc = 0;
        if (power <= 0) {
          power = 1;
        }
        if (maxpower <= 0) {
          maxpower = 1;
        }
        perc = (power * 100) / maxpower;
        if (perc > 100) {
          prompt += str_format("@D[@GSTA@n@Y: @G{:d}{}@D]@n", perc, "@w%");
        } else if (perc >= 70) {
          prompt += str_format("@D[@GSTA@n@Y: @c{:d}{}@D]@n", perc, "@w%");
        } else if (perc >= 51) {
          prompt += str_format("@D[@GSTA@n@Y: @Y{:d}{}@D]@n", perc, "@w%");
        } else if (perc >= 20) {
          prompt += str_format("@D[@GSTA@n@Y: @y{:d}{}@D]@n", perc, "@w%");
        } else {
          prompt += str_format("@D[@GSTA@n@Y: @r{:d}{}@D]@n", perc, "@w%");
        }
      }
      if (char_prf_flagged(g, d->character, PRF_DISPTNL) && char_get_level(d->character) < 100) {
        prompt += str_format("@D[@yTNL@Y: @W{}@D]@n", add_commas(level_exp(d->character, char_get_level(d->character) + 1) - char_get_exp(d->character)));
      }
      if (char_prf_flagged(g, d->character, PRF_DISTIME)) {
        prompt += str_format("@D[@W{:>2d} {}@D]@n", (g->time_info.hours % 12 == 0) ? 12 : (g->time_info.hours % 12), g->time_info.hours >= 12 ? "PM" : "AM");
      }
      if (char_prf_flagged(g, d->character, PRF_DISGOLD)) {
        prompt += str_format("@D[@YZen@y: @W{}@D]@n", add_commas(char_get_gold(d->character)));
      }
      if (char_prf_flagged(g, d->character, PRF_DISPRAC)) {
        prompt += str_format("@D[@mPS@y: @W{}@D]@n", add_commas(char_get_practice_points(g, d->character)));
      }
      if (char_prf_flagged(g, d->character, PRF_DISHUTH)) {
        int hun = char_get_conditions(g, d->character, HUNGER), thir = char_get_conditions(g, d->character, THIRST);
        prompt += "\n@D[@mHung@y:";
        if (hun >= 48) {
          prompt += " @WFull@D]@n";
        } else if (hun >= 40) {
          prompt += " @WAlmost Full@D]@n";
        } else if (hun >= 30) {
          prompt += " @WNeed Snack@D]@n";
        } else if (hun >= 20) {
          prompt += " @WHungry@D]@n";
        } else if (hun >= 15) {
          prompt += " @WVery Hungry@D]@n";
        } else if (hun >= 10) {
          prompt += " @WAlmost Starving@D]@n";
        } else if (hun >= 5) {
          prompt += " @WNear Starving@D]@n";
        } else if (hun >= 0) {
          prompt += " @WStarving@D]@n";
        } else {
          prompt += " @WN/A@D]@n";
        }
        if (thir >= 48) {
          prompt += "@D[@mThir@y: @WQuenched@D]@n";
        } else if (thir >= 40) {
          prompt += "@D[@mThir@y: @WNeed Sip@D]@n";
        } else if (thir >= 30) {
          prompt += "@D[@mThir@y: @WNeed Drink@D]@n";
        } else if (thir >= 20) {
          prompt += "@D[@mThir@y: @WThirsty@D]@n";
        } else if (thir >= 15) {
          prompt += "@D[@mThir@y: @WVery Thirsty@D]@n";
        } else if (thir >= 10) {
          prompt += "@D[@mThir@y: @WAlmost Dehydrated@D]@n";
        } else if (thir >= 5) {
          prompt += "@D[@mThir@y: @WNear Dehydration@D]@n";
        } else if (thir >= 0) {
          prompt += "@D[@mThir@y: @WDehydrated@D]@n";
        } else {
          prompt += "@D[@mThir@y: @WN/A@D]@n";
        }
      }
      if (has_group(g, d->character) && !char_prf_flagged(g, d->character, PRF_GHEALTH)) {
        prompt += str_format("\n{}", report_party_health(g, d->character));
      }
      prompt += "\n";
    }
    if (prompt.size() < 5)
      prompt += ">\n";

  } else if (desc_get_connected(d) == CON_PLAYING && char_is_npc(d->character)) {
    std::string name(char_get_name(d->character));
    prompt           = str_format("{}>\n", CAP(name));
  }

  if (prompt.size() >= MAX_PROMPT_LENGTH)
    prompt.resize(MAX_PROMPT_LENGTH - 1); /* the room process_output() keeps for it */
  return prompt;
}
