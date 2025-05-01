#include "simulationcraft.hpp"

#include "sc_warlock_pets.hpp"

#include "sc_warlock.hpp"

namespace warlock
{
warlock_pet_t::warlock_pet_t( warlock_t* owner, util::string_view pet_name, pet_e pt, bool guardian )
  : pet_t( owner->sim, owner, pet_name, pt, guardian ),
    special_action( nullptr ),
    melee_attack( nullptr ),
    summon_stats( nullptr ),
    buffs()
{
  owner_coeff.ap_from_sp = 0.5;
  owner_coeff.sp_from_sp = 1.0;
  owner_coeff.health = 0.5;

  register_on_arise_callback( this, [ owner ]() { owner->active_pets++; } );
  register_on_demise_callback( this, [ owner ]( const player_t* ) { owner->active_pets--; } );
}

warlock_t* warlock_pet_t::o()
{ return static_cast<warlock_t*>( owner ); }

const warlock_t* warlock_pet_t::o() const
{ return static_cast<warlock_t*>( owner ); }

void warlock_pet_t::create_buffs()
{
  pet_t::create_buffs();

  // Demonology
  // TODO: MOP - these should all exist?
  // buffs.demonic_strength = make_buff( this, "demonic_strength", o()->talents.demonic_strength )
  //                              ->set_default_value( o()->talents.demonic_strength->effectN( 2 ).percent() )
  //                              ->set_cooldown( 0_ms );

  // buffs.grimoire_of_service = make_buff( this, "grimoire_of_service", o()->talents.grimoire_of_service )
  //                                 ->set_default_value_from_effect( 1 );

  // buffs.demonic_power = make_buff( this, "demonic_power", o()->talents.demonic_power_buff )
  //                           ->set_default_value_from_effect( 5 );

  // All Specs

  // To avoid clogging the buff reports, we silence the pet movement statistics since Implosion uses them regularly
  // and there are a LOT of Wild Imps. We can instead lump them into a single tracking buff on the owner.
  player_t::buffs.movement->quiet = true;
  assert( player_t::buffs.movement->stack_change_callback.empty() );
  player_t::buffs.movement->set_stack_change_callback( [ this ]( buff_t*, int prev, int cur )
                            {
                              if ( cur > prev )
                                o()->buffs.pet_movement->trigger();
                              else if ( cur < prev )
                                o()->buffs.pet_movement->decrement();
                            } );

  // These buffs are needed for operational purposes but serve little to no reporting purpose
  buffs.demonic_strength->quiet = true;
  buffs.grimoire_of_service->quiet = true;
  buffs.demonic_power->quiet = true;
}

void warlock_pet_t::init_base_stats()
{
  pet_t::init_base_stats();

  resources.base[ RESOURCE_ENERGY ] = 200;
  resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10;

  base.spell_power_per_intellect = 1.0;

  intellect_per_owner = 0;
  stamina_per_owner   = 0;

  main_hand_weapon.type = WEAPON_BEAST;
  main_hand_weapon.swing_time = 2_s;
}

void warlock_pet_t::init_action_list()
{
  if ( special_action )
  {
    if ( type == PLAYER_PET )
      special_action->background = true;
    else
      special_action->action_list = get_action_priority_list( "default" );
  }

  pet_t::init_action_list();

  if ( summon_stats )
    for ( size_t i = 0; i < action_list.size(); ++i )
      summon_stats->add_child( action_list[ i ]->stats );
}

void warlock_pet_t::schedule_ready( timespan_t delta_time, bool waiting )
{
  dot_t* d;
  if ( melee_attack && !melee_attack->execute_event &&
       ( melee_on_summon || !debug_cast<pets::warlock_pet_melee_t*>( melee_attack )->first ) &&
       !( special_action && ( d = special_action->get_dot() ) && d->is_ticking() ) )
  {
    melee_attack->schedule_execute();
  }

  pet_t::schedule_ready( delta_time, waiting );
}

double warlock_pet_t::composite_player_multiplier( school_e school ) const
{
  double m = pet_t::composite_player_multiplier( school );

  m *= 1.0 + buffs.grimoire_of_service->check_value();

  m *= 1.0 + buffs.demonic_power->check_value();

  return m;
}

double warlock_pet_t::composite_player_critical_damage_multiplier( const action_state_t* s ) const
{
  double m = pet_t::composite_player_critical_damage_multiplier( s );

  // Handled in pet_t::composite_player_critical_damage_multiplier now, as pets inherit these modifiers from the owner. 
  // m += o()->talents.demonic_brutality->effectN( 1 ).percent();

  return m;
}

double warlock_pet_t::composite_spell_haste() const
{
  double m = pet_t::composite_spell_haste();

  return m;
}

double warlock_pet_t::composite_spell_cast_speed() const
{
  double m = pet_t::composite_spell_cast_speed();

  return m;
}

double warlock_pet_t::composite_melee_auto_attack_speed() const
{
  double m = pet_t::composite_melee_auto_attack_speed();

  return m;
}

void warlock_pet_t::arise()
{
  if ( melee_attack && melee_on_summon )
    melee_attack->reset();

  pet_t::arise();
}

void warlock_pet_t::demise()
{
  pet_t::demise();

  if ( melee_attack )
    melee_attack->reset();
}

warlock_pet_td_t::warlock_pet_td_t( player_t* target, warlock_pet_t& p ) :
  actor_target_data_t( target, &p ), pet( p )
{
  // TODO: Add Whiplash to base warlock data
  debuff_whiplash = make_buff( *this, "whiplash", pet.o()->find_spell( 6360 ) )
                        ->set_default_value( pet.o()->find_spell( 6360 )->effectN( 2 ).percent() )
                        ->set_max_stack( pet.o()->find_spell( 6360 )->max_stacks() - 1 ); // Data erroneously has 11 as the maximum stack
}

namespace pets
{
warlock_simple_pet_t::warlock_simple_pet_t( warlock_t* owner, util::string_view pet_name, pet_e pt )
  : warlock_pet_t( owner, pet_name, pt, true ), special_ability( nullptr )
{ resource_regeneration = regen_type::DISABLED; }

timespan_t warlock_simple_pet_t::available() const
{
  if ( !special_ability || !special_ability->cooldown )
    return warlock_pet_t::available();

  timespan_t cd_remains = special_ability->cooldown->ready - sim->current_time();
  
  if ( cd_remains <= 1_ms )
    return warlock_pet_t::available();

  return cd_remains;
}

namespace base
{

/// Felhunter Begin

felhunter_pet_t::felhunter_pet_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_FELHUNTER, false )
{
  action_list_str = "shadow_bite";

  is_main_pet = true;
}

struct spell_lock_t : public warlock_pet_spell_t
{
  spell_lock_t( warlock_pet_t* p, util::string_view options_str )
    : warlock_pet_spell_t( "Spell Lock", p, p->find_spell( 19647 ) ) // TODO: Add Spell Lock to base warlock data
  {
    parse_options( options_str );

    may_miss = may_block = may_dodge = may_parry = false;
    ignore_false_positive = is_interrupt = true;
  }
};

void felhunter_pet_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  owner_coeff.ap_from_sp = 0.575;
  owner_coeff.sp_from_sp = 1.15;

  melee_attack = new warlock_pet_melee_t( this );
  special_action = new spell_lock_t( this, "" );
}

action_t* felhunter_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "shadow_bite" )
    return new warlock_pet_melee_attack_t( this, "Shadow Bite" );
  if ( name == "spell_lock" )
    return new spell_lock_t( this, options_str );

  return warlock_pet_t::create_action( name, options_str );
}

/// Felhunter End

/// Imp Begin

imp_pet_t::imp_pet_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_IMP, false ), firebolt_cost( find_spell( 3110 )->cost( POWER_ENERGY ) ) // TODO: Add imp firebolt to base warlock data
{
  action_list_str = "firebolt";

  owner_coeff.ap_from_sp = 0.625;
  owner_coeff.sp_from_sp = 1.25;
  owner_coeff.health = 0.45;

  is_main_pet = true;
}

action_t* imp_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "firebolt" )
    return new warlock_pet_spell_t( "Firebolt", this, this->find_spell( 3110 ) );

  return warlock_pet_t::create_action( name, options_str );
}

timespan_t imp_pet_t::available() const
{
  double deficit = resources.current[ RESOURCE_ENERGY ] - firebolt_cost;

  if ( deficit >= 0 )
    return warlock_pet_t::available();

  double time_to_threshold = std::fabs( deficit ) / resource_regen_per_second( RESOURCE_ENERGY );

  // Fuzz regen by making the pet wait a bit extra if it's just below the resource threshold
  if ( time_to_threshold < 0.001 )
    return warlock_pet_t::available();

  return timespan_t::from_seconds( time_to_threshold );
}

/// Imp End

/// Sayaad Begin

sayaad_pet_t::sayaad_pet_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_SAYAAD, false )
{
  main_hand_weapon.swing_time = 3_s;
  action_list_str = "whiplash/lash_of_pain";

  is_main_pet = true;
}

void sayaad_pet_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  owner_coeff.ap_from_sp = 0.575;
  owner_coeff.sp_from_sp = 1.15;

  melee_attack = new warlock_pet_melee_t( this );
}

struct whiplash_t : public warlock_pet_spell_t
{
  whiplash_t( warlock_pet_t* p ) : warlock_pet_spell_t( p, "Whiplash" )
  { }

  void impact( action_state_t* s ) override
  {
    warlock_pet_spell_t::impact( s );

    pet_td( s->target )->debuff_whiplash->trigger();
  }
};

double sayaad_pet_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = warlock_pet_t::composite_player_target_multiplier( target, school );

  m *= 1.0 + get_target_data( target )->debuff_whiplash->check_stack_value();

  return m;
}

action_t* sayaad_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "lash_of_pain" )
    return new warlock_pet_spell_t( this, "Lash of Pain" );
  if ( name == "whiplash" )
    return new whiplash_t( this );

  return warlock_pet_t::create_action( name, options_str );
}

/// Sayaad End

/// Voidwalker Begin

voidwalker_pet_t::voidwalker_pet_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_VOIDWALKER, false )
{
  action_list_str = "consuming_shadows";

  is_main_pet = true;
}

struct consuming_shadows_t : public warlock_pet_spell_t
{
  consuming_shadows_t( warlock_pet_t* p ) 
    : warlock_pet_spell_t( p, "Consuming Shadows" )
  {
    aoe = -1;
    may_crit = false;
  }
};

void voidwalker_pet_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  owner_coeff.ap_from_sp = 0.575;
  owner_coeff.sp_from_sp = 1.15;
  owner_coeff.health = 0.7;

  melee_attack = new warlock_pet_melee_t( this );
}

action_t* voidwalker_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "consuming_shadows" )
    return new consuming_shadows_t( this );

  return warlock_pet_t::create_action( name, options_str );
}

/// Voidwalker End


/// Infernal Begin

infernal_t::infernal_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_INFERNAL, true ), immolation( nullptr )
{
  resource_regeneration = regen_type::DISABLED;

  type = MAIN;

  owner_coeff.ap_from_sp = 1.65;
  owner_coeff.sp_from_sp = 1.65;
}

struct immolation_tick_t : public warlock_pet_spell_t
{
  immolation_tick_t( warlock_pet_t* p )
    : warlock_pet_spell_t( "Immolation", p /* TODO: add this in */ )
  {
    aoe = -1;
    background = may_crit = true;
  }
};

struct infernal_melee_t : warlock_pet_melee_t
{
  infernal_melee_t( warlock_pet_t* p, double wm, const char* name = "melee" ) :
    warlock_pet_melee_t ( p, wm, name )
  { }
};

void infernal_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  melee_attack = new infernal_melee_t( this, 1.0 );
}

void infernal_t::create_buffs()
{
  warlock_pet_t::create_buffs();

  auto damage = new immolation_tick_t( this );

  immolation = make_buff<buff_t>( this, "immolation" /* TODO: fix this */ )
                   ->set_tick_callback( [ damage, this ]( buff_t*, int, timespan_t ) {
                        damage->execute_on_target( target );
                     } );
}

void infernal_t::arise()
{
  warlock_pet_t::arise();

  // 2024-07-18 Testing indicates there is a delay after spawn before first melee
  // Embers looks to trigger at around the same time as first melee swing, but Immolation takes longer to apply (and has no zero-tick)
  // Additionally, there is some unknown amount of movement adjustment the pet can take, so we model this with a distribution
  timespan_t delay = rng().gauss<1000,100>();

  make_event( *sim, delay, [ this ] {
    buffs.embers->trigger();

    melee_attack->set_target( target );
    melee_attack->schedule_execute();
  } );

  make_event( *sim, delay + 750_ms, [ this ] {
    immolation->trigger();
  } );
}

void infernal_t::demise()
{
  warlock_pet_t::demise();
}

double infernal_t::composite_player_multiplier( school_e school ) const
{
  double m = warlock_pet_t::composite_player_multiplier( school );

  return m;
}
/// Infernal End

/// Doomguard Begin
struct doom_bolt_t : public warlock_pet_spell_t
{
  doom_bolt_t( warlock_pet_t* p )
    : warlock_pet_spell_t( "Doom Bolt", p /*TODO: fix this MOP*/ )
  { }

  double cost_pct_multiplier() const override
  { return 0.0; }

  void execute() override
  {
    if ( debug_cast<doomguard_t*>( p() )->doom_bolt_executes <= 0 )
    {
      make_event( sim, 0_ms, [ this ]() { player->cast_pet()->dismiss(); } );
      return;
    }

    warlock_pet_spell_t::execute();

    debug_cast<doomguard_t*>( p() )->doom_bolt_executes--;
  }
};

doomguard_t::doomguard_t( warlock_t* owner )
  : warlock_simple_pet_t( owner, "Doomguard", PET_DOOMGUARD )
{
  action_list_str = "travel/doom_bolt";

  owner_coeff.ap_from_sp = 1.0;
  owner_coeff.sp_from_sp = 1.0;
}

void doomguard_t::init_base_stats()
{
  warlock_simple_pet_t::init_base_stats();

  special_ability = new doom_bolt_t( this );
}

action_t* doomguard_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "doom_bolt" )
    return new doom_bolt_t( this );

  return warlock_simple_pet_t::create_action( name, options_str );
}

void doomguard_t::arise()
{
  warlock_simple_pet_t::arise();

  // doom_bolt_executes = as<int>( o()->talents.pact_of_the_eredruin->effectN( 1 ).base_value() );
}

/// Doomguard End


}  // namespace base

namespace demonology
{

/// Felguard Begin

felguard_pet_t::felguard_pet_t( warlock_t* owner, util::string_view name )
  : warlock_pet_t( owner, name, PET_FELGUARD, false ),
    soul_strike( nullptr ),
    felguard_guillotine( nullptr ),
    hatred_proc( nullptr ),
    demonic_strength_executes( 0 ),
    min_energy_threshold( find_spell( 89751 )->cost( POWER_ENERGY ) ),
    max_energy_threshold( 100 )
{
  action_list_str = "travel";

  action_list_str += "/felstorm_demonic_strength";
  if ( !owner->disable_auto_felstorm )
    action_list_str += "/felstorm";
  action_list_str += "/legion_strike,if=energy>=" + util::to_string( max_energy_threshold );

  felstorm_cd = get_cooldown( "felstorm" );
  dstr_cd = get_cooldown( "felstorm_demonic_strength" );

  owner_coeff.health = 0.75;

  is_main_pet = true;
}

struct felguard_melee_t : public warlock_pet_melee_t
{
  felguard_melee_t( warlock_pet_t* p, double wm, const char* name = "melee" ) :
    warlock_pet_melee_t ( p, wm, name )
  {
  }

  void impact( action_state_t* s ) override
  {
    auto amount = s->result_raw;

    warlock_pet_melee_t::impact( s );
  }
};

struct axe_toss_t : public warlock_pet_spell_t
{
  axe_toss_t( warlock_pet_t* p, util::string_view options_str )
    : warlock_pet_spell_t( "Axe Toss", p, p->find_spell( 89766 ) )
  {
    parse_options( options_str );

    may_miss = may_block = may_dodge = may_parry = false;
    ignore_false_positive = is_interrupt = true;
  }
};

struct legion_strike_t : public warlock_pet_melee_attack_t
{
  bool main_pet;

  legion_strike_t( warlock_pet_t* p, util::string_view options_str ) 
    : warlock_pet_melee_attack_t( p, "Legion Strike" )
  {
    parse_options( options_str );
    aoe = -1;
    weapon = &( p->main_hand_weapon );
    main_pet = true;
  }

  legion_strike_t( warlock_pet_t* p, util::string_view options_str, bool is_main_pet )
    : legion_strike_t( p, options_str )
  { main_pet = is_main_pet; }

  void execute() override
  {
    warlock_pet_melee_attack_t::execute();

    // p()->buffs.empowered_legion_strike->decrement();
  }

  double action_multiplier() const override
  {
    double m = warlock_pet_melee_attack_t::action_multiplier();

    return m;
  }
};

struct felstorm_t : public warlock_pet_melee_attack_t
{
  struct felstorm_tick_t : public warlock_pet_melee_attack_t
  {
    bool applies_fel_sunder;

    felstorm_tick_t( warlock_pet_t* p, const spell_data_t *s )
      : warlock_pet_melee_attack_t( "Felstorm (tick)", p, s )
    {
      aoe = -1;
      reduced_aoe_targets = data().effectN( 3 ).base_value();
      background = true;
      weapon = &( p->main_hand_weapon );
      applies_fel_sunder = false;
    }

    double action_multiplier() const override
    {
      double m = warlock_pet_melee_attack_t::action_multiplier();
      
      m *= 1.0 + p()->buffs.demonic_strength->check_value();

      return m;
    }

    void impact( action_state_t* s ) override
    {
      warlock_pet_melee_attack_t::impact( s );
    }
  };

  felstorm_t( warlock_pet_t* p, util::string_view options_str, const std::string n = "Felstorm" )
    : warlock_pet_melee_attack_t( n, p, p->find_spell( 89751 ) )
  {
    parse_options( options_str );
    tick_zero = true;
    hasted_ticks = true;
    may_miss = false;
    may_crit = false;
    channeled = true;

    dynamic_tick_action = true;
    tick_action = new felstorm_tick_t( p, p->find_spell( 89753 ));
  }

  felstorm_t( warlock_pet_t* p, util::string_view options_str, bool main_pet, const std::string n = "Felstorm" )
    : felstorm_t( p, options_str, n )
  {
    // 2024-07-14 GFG Felstorm applies Fel Sunder, possibly bug
    if ( !main_pet )
      cooldown->duration = 45_s; // 2022-11-11: GFG does not appear to cast a second Felstorm even if the cooldown would come up, so we will pad this value to be longer than the possible duration.

    if ( main_pet )
      internal_cooldown = p->o()->get_cooldown( "felstorm_icd" );
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  { return s->action->tick_time( s ) * 5.0; }

  void execute() override
  {
    warlock_pet_melee_attack_t::execute();

    // New in 10.0.5 - Hardcoded scripted shared cooldowns while one of Felstorm, Demonic Strength, or Guillotine is active
    if ( internal_cooldown )
      internal_cooldown->start( 5_s * p()->composite_spell_haste() );

    p()->melee_attack->cancel();
  }
};

struct demonic_strength_t : public felstorm_t
{
  demonic_strength_t( warlock_pet_t* p, util::string_view options_str )
    : felstorm_t( p, options_str, std::string( "Felstorm (Demonic Strength)" ) )
  {
  }

  void execute() override
  {
    warlock_pet_melee_attack_t::execute();
    debug_cast< felguard_pet_t* >( p() )->demonic_strength_executes--;
    p()->melee_attack->cancel();
  }

  void last_tick( dot_t* d ) override
  {
    warlock_pet_melee_attack_t::last_tick( d );

    p()->buffs.demonic_strength->expire();
  }

  // 2022-10-03 - Triggering Demonic Strength seems to ignore energy cost for Felstorm
  double cost() const override
  { return 0.0; }

  bool ready() override
  {
    if ( debug_cast< felguard_pet_t* >( p() )->demonic_strength_executes <= 0 )
      return false;

    return warlock_pet_melee_attack_t::ready();
  }
};

timespan_t felguard_pet_t::available() const
{
  double energy_threshold = max_energy_threshold;
  double time_to_felstorm = ( felstorm_cd->ready - sim->current_time() ).total_seconds();
  double time_to_threshold = 0;
  double time_to_next_event = 0;

  if ( time_to_felstorm <= 0 )
    energy_threshold = min_energy_threshold;

  double deficit = resources.current[ RESOURCE_ENERGY ] - energy_threshold;

  // Not enough energy, figure out how many milliseconds it'll take to get
  if ( deficit < 0 )
    time_to_threshold = util::ceil( std::fabs( deficit ) / resource_regen_per_second( RESOURCE_ENERGY ), 3 );

  // Fuzz regen by making the pet wait a bit extra if it's just below the resource threshold
  if ( time_to_threshold < 0.001 )
    return warlock_pet_t::available();

  // Next event is either going to be the time to felstorm, or the time to gain enough energy for a
  // threshold value

  if ( time_to_felstorm <= 0 )
    time_to_next_event = time_to_threshold;
  else
    time_to_next_event = std::min( time_to_felstorm, time_to_threshold );

  if ( sim->debug )
  {
    sim->out_debug.print( "{} waiting, deficit={}, threshold={}, t_threshold={}, t_felstorm={} t_wait={}", name(),
                          deficit, energy_threshold, time_to_threshold, time_to_felstorm, time_to_next_event );
  }

  if ( time_to_next_event < 0.001 )
    return warlock_pet_t::available();
  else
    return timespan_t::from_seconds( time_to_next_event );
}

void felguard_pet_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  // Felguard is the only warlock pet type to use an actual weapon.
  main_hand_weapon.type = WEAPON_AXE_2H;
  melee_attack = new felguard_melee_t( this, 1.0, "melee" );

  // 2023-09-20: Validated coefficients
  owner_coeff.ap_from_sp = 0.9487;
  owner_coeff.sp_from_sp = 1.4519;

  melee_attack->base_dd_multiplier *= 1.42;

  special_action = new axe_toss_t( this, "" );

}

action_t* felguard_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "legion_strike" )
    return new legion_strike_t( this, options_str );
  if ( name == "felstorm" )
    return new felstorm_t( this, options_str, true );
  if ( name == "axe_toss" )
    return new axe_toss_t( this, options_str );
  if ( name == "felstorm_demonic_strength" )
    return new demonic_strength_t( this, options_str );

  return warlock_pet_t::create_action( name, options_str );
}

void felguard_pet_t::queue_ds_felstorm()
{
  demonic_strength_executes++;

  if ( !readying && !channeling && !executing )
    schedule_ready();
}

void felguard_pet_t::arise()
{
  warlock_pet_t::arise();

}

double felguard_pet_t::composite_player_multiplier( school_e school ) const
{
  double m = warlock_pet_t::composite_player_multiplier( school );

  return m;
}

double felguard_pet_t::composite_melee_auto_attack_speed() const
{
  double m = warlock_pet_t::composite_melee_auto_attack_speed();

  return m;
}

double felguard_pet_t::composite_melee_crit_chance() const
{
  double m = warlock_pet_t::composite_melee_crit_chance();

  return m;
}

double felguard_pet_t::composite_spell_crit_chance() const
{
  double m = warlock_pet_t::composite_spell_crit_chance();

  return m;
}

/// Felguard End

/// Grimoire: Felguard Begin

grimoire_felguard_pet_t::grimoire_felguard_pet_t( warlock_t* owner )
  : warlock_pet_t( owner, "grimoire_felguard", PET_SERVICE_FELGUARD, true ),
    min_energy_threshold( find_spell( 89751 )->cost( POWER_ENERGY ) ),
    max_energy_threshold( 100 )
{
  action_list_str = "travel";
  action_list_str += "/felstorm";
  action_list_str += "/legion_strike,if=energy>=" + util::to_string( max_energy_threshold );

  felstorm_cd = get_cooldown( "felstorm" );

  owner_coeff.health = 0.75;
}

 void grimoire_felguard_pet_t::arise()
 {
   warlock_pet_t::arise();

   buffs.grimoire_of_service->trigger();
 }

 void grimoire_felguard_pet_t::demise()
 {
   warlock_pet_t::demise();

 }

 double grimoire_felguard_pet_t::composite_player_multiplier( school_e school ) const
 {
   double m = warlock_pet_t::composite_player_multiplier( school );

   return m;
 }

 // TODO: Grimoire: Felguard only does a single Felstorm at most, rendering some of this unnecessary
timespan_t grimoire_felguard_pet_t::available() const
{
  double energy_threshold = max_energy_threshold;
  double time_to_felstorm = ( felstorm_cd->ready - sim->current_time() ).total_seconds();
  
  if ( time_to_felstorm <= 0 )
    energy_threshold = min_energy_threshold;

  double deficit = resources.current[ RESOURCE_ENERGY ] - energy_threshold;
  double time_to_threshold = 0;

  // Not enough energy, figure out how many milliseconds it'll take to get
  if ( deficit < 0 )
    time_to_threshold = util::ceil( std::fabs( deficit ) / resource_regen_per_second( RESOURCE_ENERGY ), 3 );

  // Fuzz regen by making the pet wait a bit extra if it's just below the resource threshold
  if ( time_to_threshold < 0.001 )
    return warlock_pet_t::available();

  // Next event is either going to be the time to felstorm, or the time to gain enough energy for a
  // threshold value
  double time_to_next_event = 0;
  if ( time_to_felstorm <= 0 )
    time_to_next_event = time_to_threshold;
  else
    time_to_next_event = std::min( time_to_felstorm, time_to_threshold );

  if ( sim->debug )
  {
    sim->out_debug.print( "{} waiting, deficit={}, threshold={}, t_threshold={}, t_felstorm={} t_wait={}", name(),
                          deficit, energy_threshold, time_to_threshold, time_to_felstorm, time_to_next_event );
  }

  if ( time_to_next_event < 0.001 )
    return warlock_pet_t::available();
  else
    return timespan_t::from_seconds( time_to_next_event );
}

void grimoire_felguard_pet_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  // Felguard is the only warlock pet type to use an actual weapon.
  main_hand_weapon.type = WEAPON_AXE_2H;
  melee_attack = new warlock_pet_melee_t( this );

  // 2023-09-20: Validated coefficients.
  owner_coeff.ap_from_sp = 0.9487;

  melee_attack->base_dd_multiplier *= 1.42;
}

action_t* grimoire_felguard_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "legion_strike" )
    return new legion_strike_t( this, options_str, false );
  if ( name == "felstorm" )
    return new felstorm_t( this, options_str, false );

  return warlock_pet_t::create_action( name, options_str );
}

/// Grimoire: Felguard End

/// Wild Imp Begin

wild_imp_pet_t::wild_imp_pet_t( warlock_t* owner )
  : warlock_pet_t( owner, "wild_imp", PET_WILD_IMP, true ), firebolt( nullptr ), power_siphon( false ), imploded( false )
{
  resource_regeneration = regen_type::DISABLED;
  owner_coeff.health = 0.15;
}

struct fel_firebolt_t : public warlock_pet_spell_t
{
  fel_firebolt_t( warlock_pet_t* p ) : warlock_pet_spell_t( "fel_firebolt", p, p->find_spell( 104318 ) )
  {
    repeating = true;
  }

  void schedule_execute( action_state_t* execute_state ) override
  {
    // We may not be able to execute anything, so reset executing here before we are going to
    // schedule anything else.
    player->executing = nullptr;

    if ( player->buffs.movement->check() || player->buffs.stunned->check() )
      return;

    warlock_pet_spell_t::schedule_execute( execute_state );
  }

  void consume_resource() override
  {
    warlock_pet_spell_t::consume_resource();

    // Imp dies if it cannot cast
    if ( player->resources.current[ RESOURCE_ENERGY ] < cost() )
      make_event( sim, 0_ms, [ this ]() { player->cast_pet()->dismiss(); } );
  }

  double cost_pct_multiplier() const override
  {
    double c = warlock_pet_spell_t::cost_pct_multiplier();

    if ( p()->o()->warlock_base.fel_firebolt_2->ok() )
      c *= 1.0 + p()->o()->warlock_base.fel_firebolt_2->effectN( 1 ).percent();


    return c;
  }

  double composite_crit_chance() const override
  {
    double m = warlock_pet_spell_t::composite_crit_chance();

    return m;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = warlock_pet_spell_t::composite_da_multiplier( s );

    return m;
  }
};

void wild_imp_pet_t::create_actions()
{
  warlock_pet_t::create_actions();

  firebolt = new fel_firebolt_t( this );
}

void wild_imp_pet_t::init_base_stats()
{
  warlock_pet_t::init_base_stats();

  resources.base[ RESOURCE_ENERGY ] = 100;
  resources.base_regen_per_second[ RESOURCE_ENERGY ] = 0;
}

void wild_imp_pet_t::reschedule_firebolt()
{
  if ( executing || is_sleeping() || player_t::buffs.movement->check() || player_t::buffs.stunned->check() )
    return;

  timespan_t gcd_adjust = gcd_ready - sim->current_time();
  if ( gcd_adjust > 0_ms )
  {
    make_event( sim, gcd_adjust, [ this ]() {
      firebolt->set_target( o()->target );
      firebolt->schedule_execute();
    } );
  }
  else
  {
    firebolt->set_target( o()->target );
    firebolt->schedule_execute();
  }
}

void wild_imp_pet_t::schedule_ready( timespan_t /* delta_time */, bool /* waiting */ )
{ reschedule_firebolt(); }

void wild_imp_pet_t::finish_moving()
{
  warlock_pet_t::finish_moving();

  reschedule_firebolt();
}

void wild_imp_pet_t::arise()
{
  warlock_pet_t::arise();

  power_siphon = false;
  imploded = false;
  o()->buffs.wild_imps->trigger();

  // Start casting fel firebolts
  firebolt->set_target( o()->target );
  firebolt->schedule_execute();
}

void wild_imp_pet_t::demise()
{
  if ( !current.sleeping )
  {
    o()->buffs.wild_imps->decrement();

    if ( expiration )
      event_t::cancel( expiration );

  }

  warlock_pet_t::demise();
}

double wild_imp_pet_t::composite_player_multiplier( school_e school ) const
{
  double m = warlock_pet_t::composite_player_multiplier( school );

  return m;
}

/// Wild Imp End

}  // namespace demonology

}  // namespace pets
}  // namespace warlock
