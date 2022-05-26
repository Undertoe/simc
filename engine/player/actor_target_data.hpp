// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "actor_pair.hpp"
#include "util/generic.hpp"

struct buff_t;
struct player_t;

struct actor_target_data_t : public actor_pair_t, private noncopyable
{
  // add the wrath raid debuffs here
  struct atd_debuff_t
  {
    buff_t * bleed_damage_amp;
    buff_t * physical_damage_amp;
    buff_t * bonus_crit_amp;
    buff_t * bonus_spellcrit_amp;
    buff_t * bonus_spellhit_amp;
    buff_t * bonus_spelldam_amp;

    buff_t * major_armor_redux;
    buff_t * minor_armor_redux;

    buff_t * judgement_of_wisdom;


  } debuff;

  struct atd_dot_t
  {
  } dot;

  actor_target_data_t( player_t* target, player_t* source );
};
