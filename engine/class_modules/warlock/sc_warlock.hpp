#pragma once
#include "simulationcraft.hpp"

#include "player/pet_spawner.hpp"
#include "sc_warlock_pets.hpp"
#include "class_modules/apl/warlock.hpp"

namespace warlock
{
struct warlock_t;

// Used for version checking in code (e.g. PTR vs Live)
enum version_check_e
{
  VERSION_PTR,
  VERSION_11_1_0,
  VERSION_ANY
};

// Finds an action with the given name. If no action exists, a new one will
// be created.
//
// Use this with secondary background actions to ensure the player only has
// one copy of the action.
template <typename Action, typename Actor, typename... Args>
action_t* get_action( util::string_view name, Actor* actor, Args&&... args )
{
  action_t* a = actor->find_action( name );
  if ( !a )
    a = new Action( name, actor, std::forward<Args>( args )... );
  assert( dynamic_cast<Action*>( a ) && a->name_str == name && a->background );
  return a;
}

struct warlock_td_t : public actor_target_data_t
{
  // Cross-spec
  propagate_const<dot_t*> dots_drain_life;
  propagate_const<dot_t*> dots_corruption;

  // Aff
  propagate_const<dot_t*> dots_agony;
  propagate_const<dot_t*> dots_seed_of_corruption;
  propagate_const<dot_t*> dots_drain_soul;
  propagate_const<dot_t*> dots_unstable_affliction;

  propagate_const<buff_t*> debuffs_haunt;

  // Demo
  propagate_const<buff_t*> debuffs_doom;

  // Destro
  propagate_const<dot_t*> dots_immolate;

  propagate_const<buff_t*> debuffs_shadowburn;
  propagate_const<buff_t*> debuffs_havoc;
  propagate_const<buff_t*> debuffs_conflagrate;

  double soc_threshold; // Aff - Seed of Corruption counts damage from cross-spec spells such as Drain Life

  warlock_t& warlock;
  warlock_td_t( player_t* target, warlock_t& p );

  void reset()
  { soc_threshold = 0; }

  void target_demise();

  int count_affliction_dots() const;
  int count_affliction_dots( bool ) const;
};

struct warlock_t : public player_t
{
public:
  player_t* havoc_target;
  player_t* ua_target; // Used for handling Unstable Affliction target swaps
  std::vector<action_t*> havoc_spells; // Used for smarter target cache invalidation.
  double agony_accumulator;
  double corruption_accumulator;
  std::vector<event_t*> wild_imp_spawns; // Used for tracking incoming imps from HoG TODO: Is this still needed with faster spawns?
  int diabolic_ritual;

  unsigned active_pets;

  // This should hold any spell data that is guaranteed in the base class or spec, without talents or other external systems required
  struct base_t
  {
    // Shared
    const spell_data_t* drain_life;
    const spell_data_t* corruption;
    const spell_data_t* shadow_bolt;

    // Affliction
    const spell_data_t* agony;
    const spell_data_t* potent_afflictions; // affli mastery
    const spell_data_t* affliction_warlock; // Spec aura

    // Demonology
    const spell_data_t* hand_of_guldan;
    const spell_data_t* hog_impact; // Secondary spell responsible for impact damage
    const spell_data_t* wild_imp; // Data for pet summoning
    const spell_data_t* fel_firebolt_2; // Still a separate spell (learned automatically). Reduces pet's energy cost
    const spell_data_t* master_demonologist; // Demonology Mastery - Increased demon damage
    const spell_data_t* demonology_warlock; // Spec aura

    // Destruction
    const spell_data_t* immolate; // Replaces Corruption
    const spell_data_t* immolate_old; // TODO: MOP Validate
    const spell_data_t* immolate_dot; // Primary spell data only contains information on direct damage
    const spell_data_t* incinerate; // Replaces Shadow Bolt
    const spell_data_t* chaos_bolt;
    const spell_data_t* chaotic_energies; // Destruction Mastery - Increased spell damage with random range
    const spell_data_t* destruction_warlock; // Spec aura
  } warlock_base;

  // Main pet held in active, guardians should be handled by pet spawners.
  struct pets_t
  {
    warlock_pet_t* active;

    spawner::pet_spawner_t<pets::infernal_t, warlock_t> infernals;
    spawner::pet_spawner_t<pets::doomguard_t, warlock_t> doomguards;

    spawner::pet_spawner_t<pets::demonology::wild_imp_pet_t, warlock_t> wild_imps;

    pets_t( warlock_t* w );
  } warlock_pet_list;

  std::vector<std::string> pet_name_list;

  // Talents
  struct talents_t
  {
    // Class Tree
    player_talent_t soulburn;
    const spell_data_t* soulburn_buff; // This buff is applied after using Soulburn and prevents another usage unless cleared

    // Specializations

    // Shared
    player_talent_t grimoire_of_sacrifice; // Aff/Destro only
    const spell_data_t* grimoire_of_sacrifice_buff; // 1 hour duration, enables proc functionality, canceled if pet summoned
    const spell_data_t* grimoire_of_sacrifice_proc; // Damage data is here, but RPPM of proc trigger is in buff data

    // Affliction
    player_talent_t unstable_affliction;
    const spell_data_t* unstable_affliction_2; // Soul Shard on demise (learned automatically)
    const spell_data_t* unstable_affliction_3; // +5 seconds to duration (learned automatically)

    // player_talent_t writhe_in_agony;
    // player_talent_t seed_of_corruption;
    // const spell_data_t* seed_of_corruption_aoe; // Explosion damage when Seed ticks

    // Demonology

    // Destruction
  } talents;

  struct proc_actions_t
  {
    // action_t* doom_proc;
    action_t* rain_of_fire_tick;
    // action_t* wicked_reaping;
  } proc_actions;

  struct tier_sets_t
  {
    // Affliction
    // const spell_data_t* hexflame_aff_2pc;
    // const spell_data_t* hexflame_aff_4pc;

    // Demonology
    // const spell_data_t* hexflame_demo_2pc;
    // const spell_data_t* hexflame_demo_4pc;

    // Destruction
    // const spell_data_t* hexflame_destro_2pc;
    // const spell_data_t* hexflame_destro_4pc;
  } tier;

  // Cooldowns - Used for accessing cooldowns outside of their respective actions, such as reductions/resets
  struct cooldowns_t
  {
    propagate_const<cooldown_t*> haunt;
    propagate_const<cooldown_t*> shadowburn;
    propagate_const<cooldown_t*> soul_fire;
    propagate_const<cooldown_t*> felstorm_icd; // Shared between Felstorm, Demonic Strength, and Guillotine TODO: Actually use this!
  } cooldowns;

  // Buffs
  struct buffs_t
  {
    // Shared Buffs
    propagate_const<buff_t*> grimoire_of_sacrifice; // Buff which grants damage proc
    propagate_const<buff_t*> soulburn;
    propagate_const<buff_t*> pet_movement; // One unified buff for some form of pet movement stat tracking

    // Affliction Buffs

    // Demonology Buffs
    propagate_const<buff_t*> demonic_core;
    propagate_const<buff_t*> wild_imps; // Buff for tracking how many Wild Imps are currently out (does NOT include imps waiting to be spawned)

    // Destruction Buffs
    propagate_const<buff_t*> backdraft;
  } buffs;

  // Gains - Many are automatically handled
  struct gains_t
  {
    // Class Talents

    // Affliction
    gain_t* corruption;
    gain_t* drain_soul;
    gain_t* unstable_affliction_refund;

    // Demonology
    // gain_t* soul_strike; // Only with Fel Invocation talent

    // Destruction
    gain_t* incinerate_crits;
    gain_t* immolate;
    gain_t* immolate_crits;
    gain_t* shadowburn_refund;
  } gains;

  // Procs
  struct procs_t
  {
    // Class Talents

    // Affliction
    // proc_t* tormented_crescendo;

    // Demonology
    proc_t* demonic_core_imps;

    // Destruction
    proc_t* conflagration_of_chaos_cf;
    proc_t* conflagration_of_chaos_sb;

  } procs;

  struct rng_settings_t
  {
    struct rng_setting_t
    {
      double setting_value;
      double default_value;
      std::string option_name;
    };

    // Affliction
    rng_setting_t agony = { 0.368, 0.368, "agony" };
    rng_setting_t nightfall = { 0.13, 0.13, "nightfall" };

    // Demonology

    // Destruction

  } rng_settings;

  int initial_soul_shards;
  std::string default_pet;
  bool disable_auto_felstorm; // For Demonology main pet
  bool normalize_destruction_mastery;
  shuffled_rng_t* rain_of_chaos_rng;
  real_ppm_t* ravenous_afflictions_rng;
  real_ppm_t* jackpot_demonology_rng;
  real_ppm_t* jackpot_destruction_rng;
  const spell_data_t* version_11_1_0_data;

  warlock_t( sim_t* sim, util::string_view name, race_e r );

  // Character Definition
  void init_spells() override;
  void init_base_stats() override;
  void create_buffs() override;
  void init_gains() override;
  void init_procs() override;
  void init_rng() override;
  void init_action_list() override;
  void init_resources( bool force ) override;
  void init_special_effects() override;
  void reset() override;
  void create_options() override;
  void add_rng_option( warlock_t::rng_settings_t::rng_setting_t& );
  int get_spawning_imp_count(); // TODO: Decide if still needed
  timespan_t time_to_imps( int count ); // TODO: Decide if still needed
  int active_demon_count() const;
  void expendables_trigger_helper( warlock_pet_t* source ); // TODO: Move to helpers?
  bool min_version_check( version_check_e version ) const;
  void create_actions() override;
  void create_affliction_proc_actions();
  void create_demonology_proc_actions();
  void create_destruction_proc_actions();
  void create_diabolist_proc_actions();
  void create_hellcaller_proc_actions();
  void create_soul_harvester_proc_actions();
  action_t* create_action( util::string_view name, util::string_view options ) override;
  pet_t* create_pet( util::string_view name, util::string_view type = {} ) override;
  void create_pets() override;
  std::string create_profile( save_e ) override;
  void copy_from( player_t* source ) override;
  resource_e primary_resource() const override { return RESOURCE_MANA; }
  role_e primary_role() const override { return ROLE_SPELL; }
  stat_e convert_hybrid_stat( stat_e s ) const override;
  double matching_gear_multiplier( attribute_e attr ) const override;
  double composite_player_multiplier( school_e school ) const override;
  double composite_player_target_multiplier( player_t* target, school_e school ) const override;
  double composite_player_pet_damage_multiplier( const action_state_t*, bool ) const override;
  double composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const override;
  void invalidate_cache( cache_e ) override;
  double composite_spell_crit_chance() const override;
  double composite_melee_crit_chance() const override;
  double composite_player_critical_damage_multiplier( const action_state_t* ) const override;
  double composite_rating_multiplier( rating_e ) const override;
  void combat_begin() override;
  void init_assessors() override;
  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override;
  std::string default_potion() const override { return warlock_apl::potion( this ); }
  std::string default_flask() const override { return warlock_apl::flask( this ); }
  std::string default_food() const override { return warlock_apl::food( this ); }
  std::string default_rune() const override { return warlock_apl::rune( this ); }
  std::string default_temporary_enchant() const override { return warlock_apl::temporary_enchant( this ); }
  void apply_affecting_auras( action_t& action ) override;
  double resource_gain( resource_e resource_type, double amount, gain_t* source = nullptr, action_t* action = nullptr ) override;
  void feast_of_souls_gain();

  target_specific_t<warlock_td_t> target_data;

  const warlock_td_t* find_target_data( const player_t* target ) const override
  { return target_data[ target ]; }

  warlock_td_t* get_target_data( player_t* target ) const override
  {
    warlock_td_t*& td = target_data[ target ];
    if ( !td )
    {
      td = new warlock_td_t( target, const_cast<warlock_t&>( *this ) );
    }
    return td;
  }

  action_t* create_action_warlock( util::string_view, util::string_view );

  action_t* create_action_affliction( util::string_view, util::string_view );
  void create_buffs_affliction();
  void init_spells_affliction();
  void init_gains_affliction();
  void init_rng_affliction();
  void init_procs_affliction();

  action_t* create_action_demonology( util::string_view, util::string_view );
  void create_buffs_demonology();
  void init_spells_demonology();
  void init_gains_demonology();
  void init_rng_demonology();
  void init_procs_demonology();

  action_t* create_action_destruction( util::string_view, util::string_view );
  void create_buffs_destruction();
  void init_spells_destruction();
  void init_gains_destruction();
  void init_rng_destruction();
  void init_procs_destruction();

  pet_t* create_main_pet( util::string_view pet_name, util::string_view pet_type );
  std::unique_ptr<expr_t> create_pet_expression( util::string_view name_str );
};

namespace helpers
{
  struct imp_delay_event_t : public player_event_t
  {
    imp_delay_event_t( warlock_t*, double, double );
    timespan_t diff;
    virtual const char* name() const override;
    virtual void execute() override;
    timespan_t expected_time();
  };

  void nightfall_updater( warlock_t* p, dot_t* d );
}
}  // namespace warlock
