/* status.c: score, status, inventory, equipment, consider
 * Split from act.informative.c (CircleMUD/DBAT; see license.doc). */
#include <array>
#include <algorithm>
#include "dbat/name_table.hpp"
#include <cstdlib>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>
#include <variant>

#include "dbat/args.hpp"
#include "dbat/cmdargs.hpp"
#include "dbat/iterate.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/utils.hpp"
#include "dbat/config.hpp"
#include "dbat/consts/songs.hpp"
#include "dbat/consts/time.hpp"
#include "dbat/help.hpp"
#include "dbat/command.hpp"
#include "dbat/affect.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/random.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/search.hpp"
#include "dbat/relocate.hpp"
#include "dbat/fileop.hpp"
#include "dbat/races.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/extract.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/vehicles.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.social.hpp"
#include "dbat/maputils.hpp"
#include "dbat/comm.hpp"
#include "dbat/spells.hpp"
#include "dbat/handler.hpp"
#include "dbat/boards.hpp"
#include "dbat/screen.hpp"
#include "dbat/mail.hpp"
#include "dbat/guild.hpp"
#include "dbat/disabled.hpp"
#include "dbat/game_world.hpp"

#include "informative_internal.hpp"

// definitions
namespace {
  /* What an arlian's evolution points can be spent on, and its cost: the level, then shares of the molt threshold and
     of the base. */
  struct evolution {
    std::string_view word, abbr; /* abbr may be empty */
    std::string_view name{};
    double of_threshold, of_base;
    int64_t (*base)(char_data *ch);
    int64_t (*gain)(char_data *ch, int64_t amt, bool trans_mult);
    std::string_view felt, label;
  };

  constexpr auto EVOLUTIONS = std::to_array<evolution>({
      {"powerlevel", "pl", "powerlevel", 0.65, 0.15, getBasePL, gainBasePL, "your body has strengthened.", "@RPL"},
      {"ki", {}, "ki", 0.50, 0.22, getBaseKI, gainBaseKI, "your spirit has strengthened.", "@CKi"},
      {"stamina", "st", "stamina", 0.50, 0.15, getBaseST, gainBaseST, "your body has more stamina.", "@GST"},
  });

  int64_t evolution_cost(struct char_data *ch, const evolution &e)
  {
    int64_t cost = char_get_level(ch);
    cost += (molt_threshold(ch) * e.of_threshold) + (e.base(ch) * e.of_base);
    return cost;
  }
} // namespace

ACMD(do_evolve)
{
  if (!char_is_arlian(ch) || char_is_npc(ch)) {
    send_to_char(g, ch, "You are not an arlian!\r\n");
    return;
  }

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "@D-=@YConvert Evolution Points To What?@D=-@n\r\n");
    send_to_char(g, ch, "@D-------------------------------------@n\r\n");
    send_to_char(g, ch, "@CPowerlevel  @D: @Y{} @Wpts\r\n", add_commas(evolution_cost(ch, EVOLUTIONS[0])));
    send_to_char(g, ch, "@CKi          @D: @Y{} @Wpts\r\n", add_commas(evolution_cost(ch, EVOLUTIONS[1])));
    send_to_char(g, ch, "@CStamina     @D: @Y{} @Wpts\r\n", add_commas(evolution_cost(ch, EVOLUTIONS[2])));
    send_to_char(g, ch, "@D[@Y{} @Wpts currently@D]@n\r\n", add_commas(char_get_moltexp(ch)));
    return;
  }

  auto it = std::ranges::find_if(EVOLUTIONS, [&](const evolution &e) { return str_eq_nocase(arg, e.word) || (!e.abbr.empty() && str_eq_nocase(arg, e.abbr)); });
  if (it == std::end(EVOLUTIONS))
    return; /* quirk: says nothing */
  int64_t cost = evolution_cost(ch, *it);
  if (cost > molt_threshold(ch)) {
    send_to_char(g, ch, "You need a few more evolution levels before you can start upgrading {}.\r\n", it->name);
    return;
  }
  if (char_get_moltexp(ch) < cost) {
    send_to_char(g, ch, "You do not have enough evolution experience.\r\n");
    return;
  }
  int64_t gain = it->base(ch) * 0.01;
  gain         = gain <= 0 ? rand_number(g, 1, 5) : rand_number(g, gain, gain * .5);
  it->gain(ch, gain, false);
  char_set_moltexp(ch, char_get_moltexp(ch) - cost);
  send_to_char(g, ch, "Your body evolves to make better use of the way it is now, and you feel that {} @D[{}@D: @Y+{}@D]@n\r\n", it->felt, it->label, add_commas(gain));
}

bool perf_skill(int skill)
{
  static constexpr auto PERFECTABLE = std::to_array<int>({464, 441, 444, 475, 474, 476, 488, 472, 485, 442, 510, 533});
  return std::ranges::contains(PERFECTABLE, skill);
}

namespace {
  void perfection_types(GameWorld *g, struct char_data *ch)
  {
    send_to_char(g, ch, "@WType @G1@D: @wOver Charged@n\r\n");
    send_to_char(g, ch, "@WType @G2@D: @wAccurate@n\r\n");
    send_to_char(g, ch, "@WType @G3@D: @wEfficient@n\r\n");
  }

  /* what perfecting a skill each way does, from type 1 */
  static constexpr auto PERFECTIONS = std::to_array<std::string_view>({
      "so that you can over charge it!",
      "so that you have supreme accuracy with it!",
      "so that you require a lower minimum charge for it!",
  });
} // namespace

ACMD(do_perf)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (char_is_npc(ch) || char_get_admlevel(ch) > 0) {
    send_to_char(g, ch, "I don't think so.\r\n");
    return;
  }
  if (arg.empty() || arg2.empty()) {
    perfection_types(g, ch);
    send_to_char(g, ch, "Syntax: perfect (skillname) (type 1/2/or 3)\r\n");
    return;
  }
  if (arg.size() < 4) {
    send_to_char(g, ch, "The skill name should be longer than 3 characters...\r\n");
    return;
  }
  /* the first skill whose name has arg in it */
  auto skills = std::views::iota(0, SKILL_TABLE_SIZE);
  auto found  = std::ranges::find_if(skills, [&](int i) { return g->spell_info[i].skilltype == skill_kind::skill && std::string_view(g->spell_info[i].name).contains(until_nul(arg)); });
  if (found == skills.end()) {
    send_to_char(g, ch, "The skill {} doesn't exist.\r\n", arg);
    return;
  }
  int skill = *found;
  if (!char_get_skill(ch, skill)) {
    send_to_char(g, ch, "You don't know {}.\r\n", arg);
    return;
  }
  if (char_get_skill(ch, skill) < 100) {
    send_to_char(g, ch, "You have not mastered the skill {} and thus can't perfect it.\r\n", arg);
    return;
  }
  if (char_get_skills_perf(ch, skill) > 0) {
    send_to_char(g, ch, "You have already mastered the skill {} and chosen how to perfect it.\r\n", arg);
    return;
  }
  if (!perf_skill(skill)) {
    send_to_char(g, ch, "You can't perfect that type of skill.\r\n");
    return;
  }
  int type = parse_int(arg2);
  if (type < 1 || type > 3) {
    perfection_types(g, ch);
    send_to_char(g, ch, "@RType must be a number between 1 and 3.@n\r\n");
    return;
  }
  send_to_char(g, ch, "You perfect the skill {} {}\r\n", g->spell_info[skill].name, PERFECTIONS[type - 1]);
  char_set_skills_perf(ch, skill, type);
}

ACMD(do_gold)
{
  if (char_get_gold(ch) == 0)
    send_to_char(g, ch, "You're broke!\r\n");
  else if (char_get_gold(ch) == 1)
    send_to_char(g, ch, "You have one little zenni.\r\n");
  else
    send_to_char(g, ch, "You have {:d} zenni.\r\n", char_get_gold(ch));
}

namespace {
  struct by_flag {
    std::optional<player_flag> flag; /* none for the last row, which fits anyone */
    std::string_view name{};
  };

  /* An android's model and version, the first flag it has; failing them all, the last. */
  constexpr auto ANDROID_MODELS = std::to_array<by_flag>({{player_flag::absorb, "@CAbsorption"}, {player_flag::repair, "@GSelf Repairing"}, {player_flag::sensem, "@RSensor Equiped"}, {std::nullopt, "@WNone"}});
  constexpr auto ANDROID_VERSIONS = std::to_array<by_flag>({{player_flag::trans1, "Beta 1.0"}, {player_flag::trans2, "ANS 2.0"}, {player_flag::trans3, "ANS 3.0"}, {player_flag::trans4, "ANS 4.0"},
                                          {player_flag::trans5, "ANS 5.0"},  {player_flag::trans6, "ANS 6.0"}, {std::nullopt, "Alpha 0.5"}});

  std::string_view first_flagged(struct char_data *ch, std::span<const by_flag> rows)
  {
    return std::ranges::find_if(rows, [&](const by_flag &r) { return !r.flag || char_plr_flagged(ch, *r.flag); })->name;
  }

  /* Whether arg is part of view's name, in lower case or with a capital. */
  bool asks_for(std::string_view view, const std::string &arg)
  {
    std::string capital(view);
    capital[0] = (char)toupper(capital[0]);
    return view.contains(arg) || std::string_view(capital).contains(arg);
  }
} // namespace

ACMD(do_score)
{
  if (char_is_npc(ch))
    return;

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  bool all      = arg.empty();
  bool personal = !all && asks_for("personal", arg);
  bool health   = !all && !personal && asks_for("health", arg);
  bool stats    = !all && !personal && !health && asks_for("statistics", arg);
  bool other    = !all && !personal && !health && !stats && asks_for("other", arg);
  if (!all && !personal && !health && !stats && !other) {
    send_to_char(g, ch, "Syntax: score, or... score (personal, health, statistics, other)\r\n");
    return;
  }

  if (all || personal) {
    send_to_char(g, ch, "  @cO@D-----------------------------[  @cPersonal  @D]-----------------------------@cO@n\n");
    send_to_char(g, ch, "  @D|  @CName@D: @W{:>15}@D,   @CTitle@D: @W{:<38}@D|@n\n", char_get_name(ch), char_get_title(ch));
    if (char_is_android(ch))
      send_to_char(g, ch, "  @D| @CModel@D: {:>15}@D,    @CUGP@D: @G{:>15}@D,  @CVersion@D: @r{:<12}@D|@n\n", first_flagged(ch, ANDROID_MODELS),
                   add_commas(char_get_upgrade(ch)), first_flagged(ch, ANDROID_VERSIONS));
    send_to_char(g, ch, "  @D|  @CRace@D: @W{:>10}@D,  @CSensei@D: @W{:>15}@D,     @CArt@D: @W{:<17}@D|@n\n", char_true_race_name(ch), char_sensei_name(ch), char_sensei_style(ch));
    std::string hei = str_format("{:d}cm", get_measure(ch, char_get_pc_height(g, ch), 0));
    std::string wei = str_format("{:d}kg", get_measure(ch, 0, char_get_pc_weight(g, ch)));
    send_to_char(g, ch, "  @D|   @CAge@D: @W{:>10}@D,  @CHeight@D: @W{:>15}@D,  @CWeight@D: @W{:<17}@D|@n\n", add_commas(char_get_age_years(g, ch)), hei, wei);
    send_to_char(g, ch, "  @D|@CGender@D: @W{:>10}@D,  @C  Size@D: @W{:>15}@D,  @C Align@D: @W{:<17}@D|@n\n", genders[(int)char_get_sex(ch)], size_names[get_size(ch)], disp_align(ch));
  }
  if (all || health) {
    send_to_char(g, ch, "  @cO@D-----------------------------@D[   @cHealth   @D]-----------------------------@cO@n\n");
    send_to_char(g, ch, "                 @D<@rPowerlevel@D>          <@BKi@D>             <@GStamina@D>@n\n");
    send_to_char(g, ch, "    @wCurrent   @D-[@R{:<16}@D]-[@R{:<16}@D]-[@R{:<16}@D]@n\n", add_commas(getCurPL(g, ch)), add_commas(getCurKI(g, ch)), add_commas(getCurST(g, ch)));
    send_to_char(g, ch, "    @wMaximum   @D-[@r{:<16}@D]-[@r{:<16}@D]-[@r{:<16}@D]@n\n", add_commas(getEffMaxPL(g, ch)), add_commas(getMaxKI(g, ch)), add_commas(getMaxST(g, ch)));
    send_to_char(g, ch, "    @wBase      @D-[@m{:<16}@D]-[@m{:<16}@D]-[@m{:<16}@D]@n\n", add_commas(getEffBasePL(g, ch)), add_commas(getEffBaseKI(g, ch)), add_commas(getEffBaseST(g, ch)));
    if (!char_is_android(ch))
      send_to_char(g, ch, "    @wLife Force@D-[@C{:>16}@D{}@c{:>16}@D]- @wLife Percent@D-[@Y{:>3d}{}@D]@n\n", add_commas(getCurLF(g, ch) > 0 ? getCurLF(g, ch) : 0), "/",
                   add_commas(getMaxLF(g, ch)), char_get_lifeperc(ch), "%");
  }
  if (all || stats) {
    send_to_char(g, ch, "  @cO@D-----------------------------@D[ @cStatistics @D]-----------------------------@cO@n\n");
    send_to_char(g, ch, "      @D<@wCharacter Level@D: @w{:<3d}@D> <@wRPP@D: @w{:<3d}@D> <@wRPP Bank@D: @w{:<3d}@D>@n\n", char_get_level(ch), char_get_rp(ch), char_get_rbank(ch));
    send_to_char(g, ch, "      @D<@wSpeed Index@D: @w{:<15}@D> <@wArmor Index@D: @w{:<15}@D>@n\n", add_commas(char_get_speedi(g, ch)), add_commas(char_get_armor(ch)));
    send_to_char(g, ch, "    @D[    @RStrength@D|@G{:>2d} ({:>3d})@D] [     @YAgility@D|@G{:>2d} ({:>3d})@D] [      @BSpeed@D|@G{:>2d} ({:>3d})@D]@n\n", char_get_real_abils_str(ch), char_get_aff_abils_str(ch), char_get_real_abils_dex(ch), char_get_aff_abils_dex(ch), char_get_real_abils_cha(ch), char_get_aff_abils_cha(ch));
    send_to_char(g, ch, "    @D[@gConstitution@D|@G{:>2d} ({:>3d})@D] [@CIntelligence@D|@G{:>2d} ({:>3d})@D] [     @MWisdom@D|@G{:>2d} ({:>3d})@D]@n\n", char_get_real_abils_con(ch), char_get_aff_abils_con(ch), char_get_real_abils_intel(ch), char_get_aff_abils_intel(ch), char_get_real_abils_wis(ch), char_get_aff_abils_wis(ch));
  }
  if (all || other) {
    send_to_char(g, ch, "  @cO@D-----------------------------@D[   @cOther    @D]-----------------------------@cO@n\n");
    send_to_char(g, ch, "                @D<@YZenni@D>                 <@rInventory Weight@D>@n\n");
    send_to_char(g, ch, "      @D[   @CCarried@D| @W{:<15}@D] [   @CCarried@D| @W{:<15}@D]@n\n", add_commas(char_get_gold(ch)), add_commas(getCurCarriedWeight(ch)));
    send_to_char(g, ch, "      @D[      @CBank@D| @W{:<15}@D] [ @CMax Carry@D| @W{:<15}@D]@n\n", add_commas(char_get_bank_gold(ch)), add_commas(getMaxCarryWeight(g, ch)));
    send_to_char(g, ch, "      @D[ @CMax Carry@D| @W{:<15}@D]@n\n", add_commas(char_gold_carry(ch)));
    send_to_char(g, ch, "      @D[  @CInterest@D| @W{:<15}@D]\n", add_commas(char_get_bank_interest(ch)));
    if (char_is_arlian(ch)) {
      send_to_char(g, ch, "                             @D<@GEvolution @D>@n\n");
      send_to_char(g, ch, "      @D[ @CEvo Level@D| @W{:<15d}@D] [   @CEvo Exp@D| @W{:<15}@D]\n", char_get_moltlevel(ch), add_commas(char_get_moltexp(ch)));
      send_to_char(g, ch, "      @D[ @CThreshold@D| @W{:<15}@D]@n\n", add_commas(molt_threshold(ch)));
    }
    if (char_get_level(ch) < 100) {
      send_to_char(g, ch, "                             @D<@gAdvancement@D>@n\n");
      send_to_char(g, ch, "      @D[@CExperience@D| @W{:<15}@D] [@CNext Level@D| @W{:<15}@D]@n\n", add_commas(char_get_exp(ch)), add_commas(level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch)));
      send_to_char(g, ch, "      @D[  @CRpp Cost@D| @W{:<15d}@D]@n\n", rpp_to_level(ch));
    }
    send_to_char(g, ch, "\n     @D<@wPlayed@D: @yYears @D(@W{:>2d}@D) @yWeeks @D(@W{:>2d}@D) @yDays @D(@W{:>2d}@D) @yHours @D(@W{:>2d}@D) @yMinutes @D(@W{:>2d}@D)>@n\n", (int)char_get_time_played(ch) / 31536000, (int)((char_get_time_played(ch) % 31536000) / 604800), (int)((char_get_time_played(ch) % 604800) / 86400), (int)((char_get_time_played(ch) % 86400) / 3600), (int)((char_get_time_played(ch) % 3600) / 60));
  }
  send_to_char(g, ch, "  @cO@D------------------------------------------------------------------------@cO@n\n");
}

namespace {
  /* A transformation a race may be in: the flag, and another it needs as well. */
  struct transformation {
    player_flag flag;
    std::optional<player_flag> also;
    std::string_view name{};
  };

  /* Rillao: transloc, add new transes here */
  constexpr auto HUMAN_FORMS = std::to_array<transformation>({{player_flag::trans1, std::nullopt, "@CSuper Human First"}, {player_flag::trans2, std::nullopt, "@CSuper Human Second"}, {player_flag::trans3, std::nullopt, "@CSuper Human Third"}, {player_flag::trans4, std::nullopt, "@CSuper Human Fourth"}});
  constexpr auto SAIYAN_FORMS = std::to_array<transformation>({{player_flag::trans1, player_flag::fpssj, "@YFull Powered @CSuper Saiyan"}, {player_flag::trans1, std::nullopt, "@CSuper Saiyan First"},
                                                {player_flag::trans2, std::nullopt, "@CSuper Saiyan Second"}, {player_flag::trans3, std::nullopt, "@CSuper Saiyan Third"},
                                                {player_flag::trans4, std::nullopt, "@CSuper Saiyan Fourth"}, {player_flag::oozaru, std::nullopt, "@COozaru"}});
  constexpr auto LEGENDARY_FORMS = std::to_array<transformation>({{player_flag::trans1, player_flag::fpssj, "@YFull Powered @CSuper Saiyan"}, {player_flag::trans1, std::nullopt, "@CSuper Saiyan First"},
                                                {player_flag::trans2, std::nullopt, "@YLegendary @CSuper Saiyan"}, {player_flag::oozaru, std::nullopt, "@COozaru"}});
  constexpr auto NAMEK_FORMS = std::to_array<transformation>({{player_flag::trans1, std::nullopt, "@CSuper Namek First"}, {player_flag::trans2, std::nullopt, "@CSuper Namek Second"}, {player_flag::trans3, std::nullopt, "@CSuper Namek Third"}, {player_flag::trans4, std::nullopt, "@CSuper Namek Fourth"}});
  constexpr auto ICER_FORMS = std::to_array<transformation>({{player_flag::trans1, std::nullopt, "@CTransform First"}, {player_flag::trans2, std::nullopt, "@CTransform Second"}, {player_flag::trans3, std::nullopt, "@CTransform Third"}, {player_flag::trans4, std::nullopt, "@CTransform Fourth"}});
  constexpr auto KONATSU_FORMS = std::to_array<transformation>({{player_flag::trans1, std::nullopt, "@CShadow First"}, {player_flag::trans2, std::nullopt, "@CShadow Second"}, {player_flag::trans3, std::nullopt, "@CShadow Third"}, {player_flag::trans4, std::nullopt, "@CShadow Fourth"}});
  constexpr auto MUTANT_FORMS = std::to_array<transformation>({{player_flag::trans1, std::nullopt, "@CMutate First"}, {player_flag::trans2, std::nullopt, "@CMutate Second"}, {player_flag::trans3, std::nullopt, "@CMutate Third"}});
  constexpr auto BIO_FORMS = std::to_array<transformation>({{player_flag::trans1, std::nullopt, "@CMature"}, {player_flag::trans2, std::nullopt, "@CSemi-perfect"}, {player_flag::trans3, std::nullopt, "@CPerfect"}, {player_flag::trans4, std::nullopt, "@CSuper Perfect"}});
  constexpr auto ANDROID_FORMS = std::to_array<transformation>({{player_flag::trans1, std::nullopt, "@CSeries 1.0"}, {player_flag::trans2, std::nullopt, "@CSeries 2.0"}, {player_flag::trans3, std::nullopt, "@CSeries 3.0"},
                                                {player_flag::trans4, std::nullopt, "@CSeries 4.0"}, {player_flag::trans5, std::nullopt, "@CSeries 5.0"}, {player_flag::trans6, std::nullopt, "@CSeries 6.0"}});
  constexpr auto MAJIN_FORMS = std::to_array<transformation>({{player_flag::trans1, std::nullopt, "@CAffinity"}, {player_flag::trans2, std::nullopt, "@CSuper"}, {player_flag::trans3, std::nullopt, "@CTrue"}});
  constexpr auto TRUFFLE_FORMS = std::to_array<transformation>({{player_flag::trans1, std::nullopt, "@CAscend First"}, {player_flag::trans2, std::nullopt, "@CAscend Second"}, {player_flag::trans3, std::nullopt, "@CAscend Third"}});
  constexpr auto KAI_FORMS = std::to_array<transformation>({{player_flag::trans1, std::nullopt, "@CMystic First"}, {player_flag::trans2, std::nullopt, "@CMystic Second"}, {player_flag::trans3, std::nullopt, "@CMystic Third"}});

  struct race_forms {
    bool (*is)(const struct char_data *ch);
    std::span<const transformation> forms;
  };

  bool full_saiyan(const struct char_data *ch) { return (char_is_saiyan(ch) || char_is_halfbreed(ch)) && !char_plr_flagged(ch, player_flag::lssj); }
  bool legendary(const struct char_data *ch) { return char_is_saiyan(ch) && char_plr_flagged(ch, player_flag::lssj); }

  /* In this order; a hoshijin's forms are its star phases, and are told apart below. */
  const auto RACE_FORMS = std::to_array<race_forms>({
      {char_is_human,     HUMAN_FORMS    },
      {full_saiyan,       SAIYAN_FORMS   },
      {legendary,         LEGENDARY_FORMS},
      {char_is_namek,     NAMEK_FORMS    },
      {char_is_icer,      ICER_FORMS     },
      {char_is_konatsu,   KONATSU_FORMS  },
      {char_is_mutant,    MUTANT_FORMS   },
      {char_is_bio,       BIO_FORMS      },
      {char_is_android,   ANDROID_FORMS  },
      {char_is_majin,     MAJIN_FORMS    },
      {char_is_truffle,   TRUFFLE_FORMS  },
      {char_is_kai,       KAI_FORMS      },
  });

  /* The form's name, with its colour; "@wNone" for none. Races are told apart in this order, a hoshijin's star
     phases first. */
  std::string_view current_form(struct char_data *ch, struct char_data *vict)
  {
    if (char_is_hoshijin(vict)) {
      if (char_get_mimic(vict) != 0 && vict != ch)
        return "@wNone";
      return char_get_starphase(vict) == 1 ? "@CBirth Phase" : char_get_starphase(vict) == 2 ? "@CLife Phase" : "@wNone";
    }
    auto race = std::ranges::find_if(RACE_FORMS, [&](const race_forms &r) { return r.is(vict); });
    if (race == std::end(RACE_FORMS))
      return "@wNone";
    auto form = std::ranges::find_if(race->forms, [&](const transformation &t) { return char_plr_flagged(vict, t.flag) && (!t.also || char_plr_flagged(vict, *t.also)); });
    return form == race->forms.end() ? "@wNone" : form->name;
  }
} // namespace

void trans_check(GameWorld *g, struct char_data *ch, struct char_data *vict)
{
  send_to_char(g, ch, "         @cCurrent Transformation@D: {}@n\r\n", current_form(ch, vict));
}

namespace {

/* What status says of the body, a section at a time, each line by the first row it fits */

struct limb_row {
  std::string_view label{};
  int index;
  player_flag healthy_flag; /* a sound limb isn't cybernetic by this */
  player_flag cyber_flag;
};
constexpr auto LIMBS = std::to_array<limb_row>({
    {"Right Arm   ", 0, player_flag::crarm, player_flag::crarm},
    {"Left Arm    ", 1, player_flag::clarm, player_flag::clarm},
    {"Right Leg   ", 2, player_flag::clarm, player_flag::crleg}, /* quirk: a sound right leg is the left arm's */
    {"Left Leg    ", 3, player_flag::clleg, player_flag::clleg},
});

void appendages_status(GameWorld *g, struct char_data *ch)
{
  if (char_plr_flagged(ch, player_flag::head))
    send_to_char(g, ch, "            @D[@cHead        @D: @GHave.          @D]@n\r\n");
  else
    send_to_char(g, ch, "            @D[@cHead        @D: @rMissing.         @D]@n\r\n");

  for (const limb_row &l : LIMBS) {
    int condition = char_get_limb_condition(ch, l.index);
    if (condition >= 50 && !char_plr_flagged(ch, l.healthy_flag))
      send_to_char(g, ch, "            @D[@c{}@D: @G{:>2d}%@D/@g100%        @D]@n\r\n", l.label, condition);
    else if (condition > 0 && !char_plr_flagged(ch, l.cyber_flag))
      send_to_char(g, ch, "            @D[@c{}@D: @rBroken @y{:>2d}%@D/@g100% @D]@n\r\n", l.label, condition);
    else if (condition > 0)
      send_to_char(g, ch, "            @D[@c{}@D: @cCybernetic @G{:>2d}%@D/@G100%@D]@n\r\n", l.label, condition);
    else
      send_to_char(g, ch, "            @D[@c{}@D: @rMissing.         @D]@n\r\n", l.label);
  }

  std::optional<player_flag> tail;
  if (char_is_saiyan(ch) || char_is_halfbreed(ch))
    tail = player_flag::stail;
  else if (char_is_icer(ch) || char_is_bio(ch))
    tail = player_flag::tail;
  if (tail) {
    if (char_plr_flagged(ch, *tail))
      send_to_char(g, ch, "            @D[@cTail        @D: @GHave.            @D]@n\r\n");
    else
      send_to_char(g, ch, "            @D[@cTail        @D: @rMissing.         @D]@n\r\n");
  }
  send_to_char(g, ch, "\r\n");
}

/* A ladder: the first row whose least the value reaches says it, or else the last word */
struct rung {
  int least;
  std::string_view says{};
};

void climb(GameWorld *g, struct char_data *ch, int value, std::span<const rung> rungs, std::string_view otherwise)
{
  auto r = std::ranges::find_if(rungs, [value](const rung &x) { return value >= x.least; });
  std::string_view says = r == rungs.end() ? otherwise : r->says;
  if (!says.empty())
    send_to_char(g, ch, "{}", says);
}

constexpr auto HUNGER_RUNGS = std::to_array<rung>({
    {48, "         You are full.\r\n"},
    {40, "         You are nearly full.\r\n"},
    {30, "         You are not hungry.\r\n"},
    {21, "         You wouldn't mind a snack.\r\n"},
    {15, "         You are slightly hungry.\r\n"},
    {10, "         You are partially hungry.\r\n"},
    {5, "         You are really hungry.\r\n"},
    {2, "         You are extremely hungry.\r\n"},
    {0, "         You are starving!\r\n"},
});
constexpr auto THIRST_RUNGS = std::to_array<rung>({
    {48, "         You are not thirsty.\r\n"},
    {40, "         You are nearly quenched.\r\n"},
    {30, "         You are not thirsty.\r\n"},
    {21, "         You wouldn't mind a drink.\r\n"},
    {15, "         You are slightly thirsty.\r\n"},
    {10, "         You are partially thirsty.\r\n"},
    {5, "         You are really thirsty.\r\n"},
    {2, "         You are extremely thirsty.\r\n"},
    {0, "         You are dehydrated!\r\n"},
});
constexpr auto RELAX_RUNGS = std::to_array<rung>({
    {465, "You are far too at ease to train hard like you should. Get out of the house more often.\r\n"},
    {233, "You are too at ease to train hard like you should. Get out of the house more often.\r\n"},
    {117, "You are a bit at ease and your training suffers. Get out of the house more often.\r\n"},
});
constexpr auto DRUNK_RUNGS = std::to_array<rung>({
    {16, "You are extremely drunk.\r\n"},
    {11, "You are pretty drunk.\r\n"},
    {5, "You are drunk.\r\n"},
    {1, "You have an alcoholic buzz.\r\n"},
});

/* A line said when a flag is set */
enum class flag_kind { plr, aff, prf, affected, spell };
struct flag_line {
  flag_kind kind;
  std::variant<int, pref_flag, aff_flag, player_flag> flag; /* a player_flag for flag_kind::plr, a pref_flag for prf, an aff_flag for aff and affected, an int (spell) for the rest */
  std::string_view says{};
};

bool has_flag(GameWorld *g, struct char_data *ch, const flag_line &f)
{
  switch (f.kind) {
    case flag_kind::plr:
      return char_plr_flagged(ch, std::get<player_flag>(f.flag));
    case flag_kind::aff:
      return char_aff_flagged(ch, std::get<aff_flag>(f.flag));
    case flag_kind::prf:
      return char_prf_flagged(g, ch, std::get<pref_flag>(f.flag));
    case flag_kind::affected:
      return is_affected(ch, std::get<aff_flag>(f.flag));
    case flag_kind::spell:
      return affected_by_spell(ch, std::get<int>(f.flag));
  }
  return false;
}

/* Whether each line's flag is of its kind's family */
constexpr bool kinds_match(std::span<const flag_line> lines)
{
  return std::ranges::all_of(lines, [](const flag_line &f) {
    switch (f.kind) {
      case flag_kind::plr:
        return std::holds_alternative<player_flag>(f.flag);
      case flag_kind::prf:
        return std::holds_alternative<pref_flag>(f.flag);
      case flag_kind::aff:
      case flag_kind::affected:
        return std::holds_alternative<aff_flag>(f.flag);
      default:
        return std::holds_alternative<int>(f.flag);
    }
  });
}

void say_flags(GameWorld *g, struct char_data *ch, std::span<const flag_line> lines)
{
  for (const flag_line &f : lines)
    if (has_flag(g, ch, f))
      send_to_char(g, ch, "{}", f.says);
}

struct preference_line {
  int preference;
  std::string_view says{};
};
constexpr auto PREFERENCES = std::to_array<preference_line>({
    {0, "         You preferred a balanced form of fighting.\r\n"},
    {PREFERENCE_KI, "         You preferred a ki dominate form of fighting.\r\n"},
    {PREFERENCE_WEAPON, "         You preferred a weapon dominate form of fighting.\r\n"},
    {PREFERENCE_H2H, "         You preferred a body dominate form of fighting.\r\n"},
    {PREFERENCE_THROWING, "         You preferred a throwing dominate form of fighting.\r\n"},
});

/* What one's "hair" is, by race */
std::string_view hair_word(struct char_data *ch)
{
  if (char_is_demon(ch) || char_is_icer(ch))
    return "horns are";
  if (char_is_majin(ch))
    return "forelock is";
  if (char_is_namek(ch))
    return "antennae are";
  return "hair is";
}

static constexpr auto FORGET_LEVELS_entries = std::to_array<std::string_view>({
    "@GRemembered Well@n", "@GRemembered Well Enough@n", "@RGetting Foggy@n", "@RHalf Forgotten@n", "@rAlmost Forgotten@n", "@rForgotten@n",
});
static constexpr name_table FORGET_LEVELS(FORGET_LEVELS_entries);

void info_status(GameWorld *g, struct char_data *ch)
{
  send_to_char(g, ch, "         @D--------------------@D[@GInfo@D]---------------------@n\r\n");
  trans_check(g, ch, ch);
  send_to_char(g, ch, "         You have died {:d} times.\r\n", char_get_dcount(ch));
  if (char_plr_flagged(ch, player_flag::noshout))
    send_to_char(g, ch, "         You have been @rmuted@n on public channels.\r\n");
  if (char_get_in_room(ch) == 9)
    send_to_char(g, ch, "         You are in punishment hell, so sad....\r\n");
  if (!char_prf_flagged(g, ch, pref_flag::hints))
    send_to_char(g, ch, "         You have hints turned off.\r\n");
  if (g->NEWSUPDATE > char_get_lastpl(ch))
    send_to_char(g, ch, "         Check the 'news', it has been updated recently.\r\n");
  if (has_mail(g, char_get_idnum(ch)))
    send_to_char(g, ch, "         Check your mail at the nearest postmaster.\r\n");
  if (char_prf_flagged(g, ch, pref_flag::hide))
    send_to_char(g, ch, "         You are hidden from who and ooc.\r\n");
  if (!char_get_voice(ch).empty())
    send_to_char(g, ch, "         Your voice desc: '{}'\r\n", char_get_voice(ch));

  int feature = char_get_distfea(ch);
  if (feature == DISTFEA_EYE)
    send_to_char(g, ch, "         Your eyes are your most distinctive feature.\r\n");
  auto preference = std::ranges::find(PREFERENCES, char_get_preference(ch), &preference_line::preference);
  if (preference != std::end(PREFERENCES))
    send_to_char(g, ch, "{}", preference->says);
  if (feature == DISTFEA_HAIR)
    send_to_char(g, ch, "         Your {} your most distinctive feature.\r\n", hair_word(ch));
  if (feature == DISTFEA_SKIN)
    send_to_char(g, ch, "         Your skin is your most distinctive feature.\r\n");
  if (feature == DISTFEA_HEIGHT)
    send_to_char(g, ch, "         Your height is your most distinctive feature.\r\n");
  if (feature == DISTFEA_WEIGHT)
    send_to_char(g, ch, "         Your weight is your most distinctive feature.\r\n");

  if (struct obj_data *scouter = char_get_equipment(ch, WEAR_EYE)) {
    if (obj_get_scoutfreq(scouter) == 0)
      obj_set_scoutfreq(scouter, 1);
    send_to_char(g, ch, "         Your scouter is on frequency @G{:d}@n\r\n", obj_get_scoutfreq(scouter));
  }
  if (char_get_charge(ch) > 0)
    send_to_char(g, ch, "         You have @C{}@n ki charged.\r\n", add_commas(char_get_charge(ch)));
  if (char_get_kaioken(ch) > 0)
    send_to_char(g, ch, "         You are focusing Kaioken x {:d}.\r\n", char_get_kaioken(ch));
  if (char_aff_flagged(ch, aff_flag::sanctuary))
    send_to_char(g, ch, "         You are surrounded by a barrier @D(@Y{}@D)@n\r\n", add_commas(char_get_barrier(ch)));
  if (char_aff_flagged(ch, aff_flag::fireshield))
    send_to_char(g, ch, "         You are surrounded by flames!@n\r\n");
  if (char_get_suppression(ch) > 0)
    send_to_char(g, ch, "         You are suppressing current PL to {:d}.\r\n", char_get_suppression(ch));
  if (char_is_majin(ch))
    send_to_char(g, ch, "         You have ingested {:d} people.\r\n", char_get_absorbs(ch));
  if (char_is_bio(ch))
    send_to_char(g, ch, "         You have {:d} absorbs left.\r\n", char_get_absorbs(ch));
  send_to_char(g, ch, "         You have {} colored aura.\r\n", aura_types[char_get_aura(ch)]);

  if (char_get_level(ch) >= 100)
    send_to_char(g, ch, "         Your strengths are potentially limitless.\r\n");
  else if ((char_is_android(ch) && char_plr_flagged(ch, player_flag::absorb)) || (!char_is_android(ch) && !char_is_bio(ch) && !char_is_majin(ch)))
    send_to_char(g, ch, "         @R{}@n to SC a stat this level.\r\n", add_commas(calc_soft_cap(ch)));
  else
    send_to_char(g, ch, "         @R{}@n in PL/KI/ST combined to SC this level.\r\n", add_commas(calc_soft_cap(ch)));

  if (char_get_forgeting(ch) != 0)
    send_to_char(g, ch, "         @MForgetting @D[@m{} - {}@D]@n\r\n", g->spell_info[char_get_forgeting(ch)].name, FORGET_LEVELS[char_get_forgetcount(ch)]);
  else
    send_to_char(g, ch, "         @MForgetting @D[@mNothing.@D]@n\r\n");

  if (char_get_skill(ch, SKILL_DAGGER) > 0) {
    if (char_get_backstabcool(ch) > 0)
      send_to_char(g, ch, "         @yYou can't preform a backstab yet.@n\r\n");
    else
      send_to_char(g, ch, "         @YYou can backstab.@n\r\n");
  }
  if (!char_get_feature(ch).empty())
    send_to_char(g, ch, "         Extra Feature: @C{}@n\r\n", char_get_feature(ch));
  if (!char_get_rdisplay(ch).empty() && char_get_rdisplay(ch) != "Empty")
    send_to_char(g, ch, "         Room Display: @C...{}@n\r\n", char_get_rdisplay(ch));
}

/* A mutant's mutations and a bio-android's genes, by genome (1 up) */
static constexpr auto MUTATIONS = std::to_array<std::string_view>({
    "Extreme Speed",  "Increased Cell Regeneration", "Extreme Reflexes", "Infravision",       "Natural Camo",
    "Limb Regen",     "Poisonous (you can use the bite attack)", "Rubbery Body", "Innate Telepathy", "Natural Energy",
});
static constexpr auto GENES = std::to_array<std::string_view>({"Human", "Saiyan", "Namek", "Icer", "Truffle", "Arlian", "Kai", "Konatsu"});

void genome_status(GameWorld *g, struct char_data *ch, std::string_view heading, std::span<const std::string_view> names, std::string_view suffix)
{
  send_to_char(g, ch, "{}", heading);
  for (int i : {0, 1}) {
    int genome = char_get_genome(ch, i);
    if (genome >= 1 && genome <= (int)names.size())
      send_to_char(g, ch, "  {}{}.\r\n", names[genome - 1], suffix);
  }
}

constexpr auto STEALTH_LINES = std::to_array<flag_line>({
    {flag_kind::plr, player_flag::eyec, "Your eyes are closed.\r\n"},
    {flag_kind::aff, aff_flag::sneak, "You are prepared to sneak where ever you go.\r\n"},
    {flag_kind::plr, player_flag::disguised, "You have disguised your facial features.\r\n"},
    {flag_kind::aff, aff_flag::flying, "You are flying.\r\n"},
    {flag_kind::plr, player_flag::piloting, "You are busy piloting a ship.\r\n"},
});
static_assert(kinds_match(STEALTH_LINES));
constexpr auto READY_LINES = std::to_array<flag_line>({
    {flag_kind::aff, aff_flag::zanzoken, "You are prepared to zanzoken.\r\n"},
    {flag_kind::aff, aff_flag::hass, "Your arms are moving fast.\r\n"},
    {flag_kind::aff, aff_flag::infuse, "Your ki will be infused in your next physical attack.\r\n"},
    {flag_kind::plr, player_flag::tailhide, "Your tail is hidden!\r\n"},
    {flag_kind::plr, player_flag::nogrow, "Your tail is no longer regrowing!\r\n"},
    {flag_kind::affected, aff_flag::special_pose, "You are feeling confident from your pose earlier.\r\n"},
    {flag_kind::affected, aff_flag::hydrozap, "You are effected by Kanso Suru.\r\n"},
});
static_assert(kinds_match(READY_LINES));
constexpr auto AFFLICTION_LINES = std::to_array<flag_line>({
    {flag_kind::aff, aff_flag::knocked, "You have been knocked unconcious!\r\n"},
    {flag_kind::aff, aff_flag::invisible, "You are invisible.\r\n"},
    {flag_kind::aff, aff_flag::detect_invis, "You are sensitive to the presence of invisible things.\r\n"},
    {flag_kind::aff, aff_flag::mbreak, "Your mind has been broken!\r\n"},
    {flag_kind::aff, aff_flag::wither, "You've been withered! You feel so weak...\r\n"},
    {flag_kind::aff, aff_flag::shocked, "Your mind has been shocked!\r\n"},
    {flag_kind::aff, aff_flag::charm, "You have been charmed!\r\n"},
    {flag_kind::spell, SPELL_MAGE_ARMOR, "You feel protected.\r\n"},
    {flag_kind::aff, aff_flag::infravision, "You can see in darkness with infravision.\r\n"},
    {flag_kind::prf, pref_flag::summonable, "You are summonable by other players.\r\n"},
    {flag_kind::aff, aff_flag::detect_align, "You see into the hearts of others.\r\n"},
    {flag_kind::aff, aff_flag::detect_magic, "You are sensitive to the magical nature of things.\r\n"},
    {flag_kind::aff, aff_flag::spirit, "You have died and are part of the SPIRIT world!\r\n"},
    {flag_kind::prf, pref_flag::nogive, "You are not accepting items being handed to you right now.\r\n"},
    {flag_kind::aff, aff_flag::ethereal, "You are ethereal and cannot interact with normal space!\r\n"},
});
static_assert(kinds_match(AFFLICTION_LINES));

/* What lasting effects are told, by their skill's name */
namespace {
struct affect_lines_entry {
  std::string_view skill{};
  std::string_view says{}; /* before " (n Mud Hours)" */
};
}
constexpr auto AFFECT_LINES = std::to_array<affect_lines_entry>({
    {"runic", "Your Kenaz rune is still in effect!"},
    {"punch", "Your Algiz rune is still in effect!"},
    {"knee", "Your Oagaz rune is still in effect!"},
    {"slam", "Your Wunjo rune is still in effect!"},
    {"heeldrop", "Your Purisaz rune is still in effect!"},
    {"special beam cannon", "Your Laguz rune is still in effect!"},
    {"might", "Your muscles are pumped!"},
    {"flex", "You are more agile right now!"},
    {"bless", "You have been blessed!"},
    {"curse", "You have been cursed!"},
    {"healing glow", "You have a healing glow enveloping your body!"},
    {"genius", "You are smarter right now!"},
    {"enlighten", "You are wiser right now!"},
    {"yoikominminken", "You have been lulled to sleep!"},
    {"solar flare", "You have been blinded!"},
    {"spirit control", "You have full control of your spirit!"},
    {"!UNUSED!", "You feel poison burning through your blood!"},
    {"tough skin", "You have toughened skin right now!"},
    {"poison", "You have been poisoned!"},
    {"warp pool", "Weakened State!"},
    {"dark metamorphosis", "Your Dark Metamorphosis is still in effect."},
    {"hayasa", "Your body has been infused to move faster!"},
});

/* Each effect is told once for a run of affects of its skill */
void affects_status(GameWorld *g, struct char_data *ch)
{
  int lasttype = 0;
  for (const affected_type &aff : char_affects(ch)) {
    std::string_view name = skill_name(g, aff.type);
    auto line             = std::ranges::find_if(AFFECT_LINES, [name](const auto &l) { return str_eq_nocase(name, l.skill); });
    if (line != std::end(AFFECT_LINES) && aff.type != lasttype) {
      lasttype = aff.type;
      send_to_char(g, ch, "{} ({:>2d} Mud Hours)\r\n", line->says, aff.duration + 1);
    }
  }
}

void sleep_status(GameWorld *g, struct char_data *ch)
{
  int sleep = char_get_sleeptime(ch);

  if (char_get_bonuses(ch, BONUS_INSOMNIAC))
    send_to_char(g, ch, "You can not sleep.\r\n");
  else if (sleep > 6 && char_get_position(ch) != POS_SLEEPING)
    send_to_char(g, ch, "You are well rested.\r\n");
  else if (sleep > 6)
    send_to_char(g, ch, "You are getting the rest you need.\r\n");
  else if (sleep > 4)
    send_to_char(g, ch, "You are rested.\r\n");
  else if (sleep > 2)
    send_to_char(g, ch, "You are not sleepy.\r\n");
  else if (sleep >= 1)
    send_to_char(g, ch, "You are getting a little sleepy.\r\n");
  else if (sleep == 0)
    send_to_char(g, ch, "You could sleep at any time.\r\n");
}

void position_status(GameWorld *g, struct char_data *ch)
{
  switch (char_get_position(ch)) {
    case POS_DEAD:
      send_to_char(g, ch, "You are DEAD!\r\n");
      break;
    case POS_MORTALLYW:
      send_to_char(g, ch, "You are mortally wounded! You should seek help!\r\n");
      break;
    case POS_INCAP:
      send_to_char(g, ch, "You are incapacitated, slowly fading away...\r\n");
      break;
    case POS_STUNNED:
      send_to_char(g, ch, "You are stunned! You can't move!\r\n");
      break;
    case POS_SLEEPING:
      send_to_char(g, ch, "You are sleeping.\r\n");
      break;
    case POS_RESTING:
      send_to_char(g, ch, "You are resting.\r\n");
      break;
    case POS_SITTING:
      send_to_char(g, ch, "You are sitting.\r\n");
      break;
    case POS_FIGHTING:
      send_to_char(g, ch, "You are fighting {}.\r\n", char_get_fighting(g, ch) ? char_pers(g, char_get_fighting(g, ch), ch) : "thin air");
      break;
    case POS_STANDING:
      send_to_char(g, ch, "You are standing.\r\n");
      break;
    default:
      send_to_char(g, ch, "You are floating.\r\n");
      break;
  }
}

void condition_status(GameWorld *g, struct char_data *ch)
{
  send_to_char(g, ch, "\r\n@D<@b-------------------------@D[@BCondition@D]@b--------------------------@D>@n\r\n");
  sleep_status(g, ch);
  climb(g, ch, char_get_relax_count(ch), RELAX_RUNGS, {});

  if (char_get_mimic(ch) > 0)
    send_to_char(g, ch, "You are mimicing the general appearance of {} {}\r\n", AN(juggleRaceName(ch, false)), juggleRaceName(ch, false));
  if (char_is_mutant(ch))
    genome_status(g, ch, "Your Mutations:\r\n", MUTATIONS, "");
  if (char_is_bio(ch))
    genome_status(g, ch, "Your genes carry:\r\n", GENES, " DNA");
  if (char_get_genome(ch, 0) == 11)
    send_to_char(g, ch, "You have used kyodaika.\r\n");
  if (char_prf_flagged(g, ch, pref_flag::noparry))
    send_to_char(g, ch, "You have decided not to parry attacks.\r\n");
  position_status(g, ch);
  if (has_group(g, ch))
    send_to_char(g, ch, "@GGroup Victories@D: @w{}@n\r\n", add_commas(char_get_combatexpertise(ch)));

  say_flags(g, ch, STEALTH_LINES);
  if (char_get_powerattack(ch) > 0)
    send_to_char(g, ch, "You are playing @y'@Y{}@y'@n.\r\n", song_types[char_get_powerattack(ch)]);
  say_flags(g, ch, READY_LINES);
  climb(g, ch, char_get_conditions(g, ch, DRUNK), DRUNK_RUNGS, {});
  affects_status(g, ch);
  say_flags(g, ch, AFFLICTION_LINES);

  if (char_get_regen(ch) > 0)
    send_to_char(g, ch, "Something is augmenting your regen rate by +{:d}%!\r\n", char_get_regen(ch));
  if (char_get_asb(ch) > 0)
    send_to_char(g, ch, "Something is augmenting your auto-skill training rate by +{:d}%!\r\n", char_get_asb(ch));
  if (char_get_raw_lifebonus(ch) > 0)
    send_to_char(g, ch, "Something is augmenting your Life Force Max by +{:d}%!\r\n", char_get_raw_lifebonus(ch));
  if (char_plr_flagged(ch, player_flag::fishing))
    send_to_char(g, ch, "Current Fishing Pole Bonus @D[@C{:d}@D]@n\r\n", char_get_accuracy(ch));
  if (char_plr_flagged(ch, player_flag::auralight))
    send_to_char(g, ch, "Aura Light is active.\r\n");
}

/* The traits, bonuses then negatives, by number */
static constexpr auto TRAITS = std::to_array<std::string_view>({
    "Thrifty     - -10% Shop Buy Cost and +10% Shop Sell Cost          ",
    "Prodigy     - +25% Experience Gained Until Level 80               ",
    "Quick Study - Character auto-trains skills faster                 ",
    "Die Hard    - Life Force's PL regen doubled, but cost is the same ",
    "Brawler     - Physical attacks do 20% more damage                 ",
    "Destroyer   - Damaged Rooms act as regen rooms for you            ",
    "Hard Worker - Physical activity bonuses + drains less stamina     ",
    "Healer      - Heal/First-aid/Vigor/Repair restore +10%            ",
    "Loyal       - +20% Experience When Grouped As Follower            ",
    "Brawny      - Strength gains +2 every 10 levels, Train STR + 75%  ",
    "Scholarly   - Intelligence gains +2 every 10 levels, Train INT + 75%",
    "Sage        - Wisdom gains +2 every 10 levels, Train WIS + 75%    ",
    "Agile       - Agility gains +2 every 10 levels, Train AGL + 75%   ",
    "Quick       - Speed gains +2 every 10 levels, Train SPD + 75%     ",
    "Sturdy      - Constitution +2 every 10 levels, Train CON + 75%    ",
    "Thick Skin  - -20% Physical and -10% ki dmg received              ",
    "Recipe Int. - Food cooked by you lasts longer/heals better        ",
    "Fireproof   - -50% Fire Dmg taken, -10% ki, immunity to burn      ",
    "Powerhitter - 15% critical hits will be x4 instead of x2          ",
    "Healthy     - 40% chance to recover from ill effects when sleeping",
    "Insomniac   - Can't Sleep. Immune to yoikominminken and paralysis ",
    "Evasive     - +15% to dodge rolls                                 ",
    "The Wall    - +20% chance to block                                ",
    "Accurate    - +20% chance to hit physical, +10% to hit with ki     ",
    "Energy Leech- -2% ki damage received for every 5 character levels,\n                  @cas long as you can take that ki to your charge pool.@D        ",
    "Good Memory - +2 Skill Slots initially, +1 every 20 levels after  ",
    /* the negatives */
    "Soft Touch  - Half damage for all hit locations                   ",
    "Late Sleeper- Can only wake automatically. 33% every hour if maxed",
    "Impulse Shop- +25% shop costs                                     ",
    "Sickly      - Suffer from harmful effects longer                  ",
    "Punching Bag- -15% to dodge rolls                                 ",
    "Pushover    - -20% block chance                                   ",
    "Poor D. Perc- -20% chance to hit with physical, -10% with ki       ",
    "Thin Skin   - +20% physical and +10% ki damage received           ",
    "Fireprone   - +50% Fire Dmg taken, +10% ki, always burned         ",
    "Energy Int. - +2% ki damage received for every 5 character levels,\n                  @rif you have ki charged you have 10% chance to lose   \n                  it and to take 1/4th damage equal to it.@D                    ",
    "Coward      - Can't Attack Enemy With 150% Your Powerlevel        ",
    "Arrogant    - Cannot Suppress                                     ",
    "Unfocused   - Charge concentration randomly breaks                ",
    "Slacker     - Physical activity drains more stamina               ",
    "Slow Learner- Character auto-trains skills slower                 ",
    "Masochistic - Defense Skills Cap At 75                            ",
    "Mute        - Can't use IC speech related commands                ",
    "Wimp        - Strength is capped at 45                            ",
    "Dull        - Intelligence is capped at 45                        ",
    "Foolish     - Wisdom is capped at 45                              ",
    "Clumsy      - Agility is capped at 45                             ",
    "Slow        - Speed is capped at 45                               ",
    "Frail       - Constitution capped at 45                           ",
    "Sadistic    - Half Experience Gained For Quick Kills              ",
    "Loner       - Can't Group with anyone, +5% train and +10% Phys    ",
    "Bad Memory  - -5 Skill Slots                                      ",
});
constexpr int FIRST_NEGATIVE = 26;

void bonus_status(GameWorld *g, struct char_data *ch)
{
  if (char_is_npc(ch))
    return;

  int count = 0;
  send_to_char(g, ch, "@CYour Traits@n\n@D-----------------------------@w\n");
  for (int i = 0; i < (int)std::size(TRAITS); i++) {
    if (i == FIRST_NEGATIVE)
      send_to_char(g, ch, "\r\n");
    if (char_get_bonuses(ch, i)) {
      send_to_char(g, ch, "@{}{}@n\n", i < FIRST_NEGATIVE ? 'c' : 'r', TRAITS[i]);
      count++;
    }
  }
  if (count == 0)
    send_to_char(g, ch, "@wNone.\r\n");
  send_to_char(g, ch, "@D-----------------------------@n\r\n");
}

} // namespace

ACMD(do_status)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!arg.empty()) {
    if (str_eq_nocase(arg, "traits"))
      bonus_status(g, ch);
    else
      send_to_char(g, ch, "The only argument status takes is 'traits'. If you just want your status do not use an argument.\r\n");
    return;
  }

  send_to_char(g, ch, "@D<@b------------------------@D[@YYour Status@D]@b-------------------------@D>@n\r\n\r\n");
  send_to_char(g, ch, "            @D---------------@CAppearance@D---------------\n");
  bringdesc(g, ch, ch);
  send_to_char(g, ch, "            @D---------------@RAppendages@D---------------\n");
  appendages_status(g, ch);

  send_to_char(g, ch, "         @D-----------------@YHunger@D/@yThirst@D-----------------@n\r\n");
  climb(g, ch, char_get_conditions(g, ch, HUNGER), HUNGER_RUNGS, "         You need not eat.\r\n");
  climb(g, ch, char_get_conditions(g, ch, THIRST), THIRST_RUNGS, "         You need not drink.\r\n");
  info_status(g, ch);
  condition_status(g, ch);
  send_to_char(g, ch, "@D<@b--------------------------------------------------------------@D>@n\r\n");
  send_to_char(g, ch, "To view your bonus/negative traits enter: status traits\r\n");
}

ACMD(do_inventory)
{
  send_to_char(g, ch, "@w              @YInventory\r\n@D-------------------------------------@w\r\n");
  if (!char_is_npc(ch) && char_plr_flagged(ch, player_flag::stolen)) {
    char_plr_set(ch, player_flag::stolen, false);
    send_to_char(g, ch, "@r   --------------------------------------------------@n\n");
    send_to_char(g, ch, "@R    You notice that you have been robbed sometime recently!\n");
    send_to_char(g, ch, "@r   --------------------------------------------------@n\n");
    return;
  }
  list_obj_to_char(g, char_get_inventory(ch), ch, SHOW_OBJ_SHORT, true);
  send_to_char(g, ch, "\n");
}

ACMD(do_equipment)
{
  send_to_char(g, ch, "        @YEquipment Being Worn\r\n@D-------------------------------------@w\r\n");
  for (int i = 1; i < NUM_WEARS; i++) {
    struct obj_data *obj = char_get_equipment(ch, i);
    bool wielded         = i == WEAR_WIELD1 || i == WEAR_WIELD2;

    if (!obj) {
      if (char_body_flagged(ch, i) && (i != WEAR_WIELD2 || !char_plr_flagged(ch, player_flag::thandw)))
        send_to_char(g, ch, "{}@wNothing.@n\r\n", wear_where[i]);
      continue;
    }
    if (!char_can_see_obj(g, ch, obj)) {
      send_to_char(g, ch, "{}", wear_where[i]);
      send_to_char(g, ch, "Something.\r\n");
      continue;
    }
    if (wielded && char_plr_flagged(ch, player_flag::thandw)) {
      send_to_char(g, ch, "@c<@CWielded by B. Hands@c>@n ");
      show_obj_to_char(g, obj, ch, SHOW_OBJ_SHORT);
      continue;
    }
    send_to_char(g, ch, "{}", wear_where[i]);
    show_obj_to_char(g, obj, ch, SHOW_OBJ_SHORT);
    if (!obj_extra_flagged(obj, extra_flag::sheath))
      continue;
    for (obj_data *thing : obj_contents(g, obj)) {
      /* quirk: the arrow is coloured on a sheath worn, not on one wielded */
      send_to_char(g, ch, wielded ? "@D  ---- @YSheathed@D ----> @n" : "@D  ---- @YSheathed@D ----@c> @n");
      show_obj_to_char(g, thing, ch, SHOW_OBJ_SHORT);
    }
  }
}

ACMD(do_levels)
{
  if (char_is_npc(ch)) {
    send_to_char(g, ch, "You ain't nothin' but a hound-dog.\r\n");
    return;
  }

  std::string buf;
  for (int i = 1; i < 100; i++)
    buf += str_format("[{:>2d}] {:>8}-{:<8} : \r\n", i, add_commas(level_exp(ch, i)), add_commas(level_exp(ch, i + 1) - 1));
  buf += str_format("[100] {:>8}          : \r\n", add_commas(level_exp(ch, 100)));
  send_to_char(g, ch, buf);
}

namespace {
  struct odds {
    int at_most; /* the victim's level less ch's */
    std::string_view says{};
  };

  constexpr auto CONSIDERED = std::to_array<odds>({
      {-10, "Now where did that chicken go?\r\n"},
      {-5,  "You could do it with a needle!\r\n"},
      {-2,  "Easy.\r\n"},
      {-1,  "Fairly easy.\r\n"},
      {0,   "The perfect match!\r\n"},
      {1,   "You could probably manage it.\r\n"},
      {2,   "You might take a beating.\r\n"},
      {3,   "You MIGHT win, maybe.\r\n"},
      {5,   "Do you feel lucky? You better.\r\n"},
      {10,  "Better bring some tough backup!\r\n"},
      {25,  "Maybe if they are allergic to you, otherwise your last words will be 'Oh shit.'\r\n"},
  });

  std::string_view pronoun(struct char_data *ch) { return char_get_sex(ch) == SEX_MALE ? "He " : (char_get_sex(ch) == SEX_FEMALE ? "She " : "It "); }
} // namespace

ACMD(do_consider)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  auto victim = find_char(g, ch, arg, find_flag::char_room, "Consider killing who?\r\n");
  if (reject(g, ch, victim))
    return;
  if (*victim == ch) {
    send_to_char(g, ch, "Easy!  Very easy indeed!\r\n");
    return;
  }
  int diff = char_get_level(*victim) - char_get_level(ch);
  auto it  = std::ranges::find_if(CONSIDERED, [&](const odds &o) { return diff <= o.at_most; });
  send_to_char(g, ch, "{}", it == std::end(CONSIDERED) ? "No chance.\r\n" : it->says);
}

ACMD(do_diagnose)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  struct char_data *vict = char_get_fighting(g, ch);
  if (!arg.empty()) {
    auto found = find_char(g, ch, arg, find_flag::char_room, CONFIG_NOPERSON);
    if (reject(g, ch, found))
      return;
    vict = *found;
  } else if (!vict) {
    send_to_char(g, ch, "Diagnose who?\r\n");
    return;
  }
  send_to_char(g, ch, "{}", pronoun(vict));
  diag_char_to_char(g, vict, ch);
}
