#include "simulationcraft.hpp"

#include "sc_warlock.hpp"

namespace warlock
{
namespace actions_destruction
{
using namespace actions;

struct destruction_spell_t : public warlock_spell_t
{
public:
  gain_t* gain;

  bool destro_mastery;

  destruction_spell_t( warlock_t* p, util::string_view n ) : destruction_spell_t( n, p, p->find_class_spell( n ) )
  {
  }

  destruction_spell_t( warlock_t* p, util::string_view n, specialization_e s )
    : destruction_spell_t( n, p, p->find_class_spell( n, s ) )
  {
  }

  destruction_spell_t( util::string_view token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() )
    : warlock_spell_t( token, p, s )
  {
    may_crit          = true;
    tick_may_crit     = true;
    weapon_multiplier = 0.0;
    gain              = player->get_gain( name_str );
    destro_mastery    = true;
  }

  void consume_resource() override
  {
    warlock_spell_t::consume_resource();

    if ( resource_current == RESOURCE_SOUL_SHARD && p()->buffs.rain_of_chaos->check() )
    {
      for ( int i = 0; i < as<int>( cost() ); i++ )
      {
        //trigger deck of cards draw
        if ( p()->rain_of_chaos_rng->trigger() )
        {
          //Currently storing infernal duration (spell 335286) in buff default value
          p()->warlock_pet_list.roc_infernals.spawn( timespan_t::from_millis( p()->buffs.rain_of_chaos->default_value ), 1U);
          p()->procs.rain_of_chaos->occur();
        }
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( p()->talents.reverse_entropy->ok() )
    {
      auto success = p()->buffs.reverse_entropy->trigger();
      if ( success )
      {
        p()->procs.reverse_entropy->occur();
      }
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = warlock_spell_t::composite_target_multiplier( t );

    auto td = this->td( t );

    if ( td->debuffs_roaring_blaze->check() && data().affected_by( td->debuffs_roaring_blaze->data().effectN( 1 ) ) )
      m *= 1.0 + td->debuffs_roaring_blaze->data().effectN( 1 ).percent();

    // SL - Legendary
    if ( td->debuffs_odr->check() && data().affected_by( td->debuffs_odr->data().effectN( 1 ) ) )
      m *= 1.0 + td->debuffs_odr->data().effectN( 1 ).percent();

    return m;
  }

  // TODO: Check order of multipliers on Havoc'd spells
  double action_multiplier() const override
  {
    double pm = warlock_spell_t::action_multiplier();

    if ( p()->mastery_spells.chaotic_energies->ok() && destro_mastery )
    {
      double destro_mastery_value = p()->cache.mastery_value() / 2.0;
      double chaotic_energies_rng = rng().range( 0, destro_mastery_value );

      pm *= 1.0 + chaotic_energies_rng + ( destro_mastery_value );
    }

    return pm;
  }
};

// Talents
struct internal_combustion_t : public destruction_spell_t
{
  internal_combustion_t( warlock_t* p )
    : destruction_spell_t( "Internal Combustion", p, p->talents.internal_combustion )
  {
    background     = true;
    destro_mastery = false;
  }

  void init() override
  {
    destruction_spell_t::init();

    snapshot_flags &= STATE_NO_MULTIPLIER;
  }

  void execute() override
  {
    warlock_td_t* td = this->td( target );
    dot_t* dot       = td->dots_immolate;

    assert( dot->current_action );
    action_state_t* state = dot->current_action->get_state( dot->state );
    dot->current_action->calculate_tick_amount( state, 1.0 );
    double tick_base_damage  = state->result_raw;
    timespan_t remaining     = std::min( dot->remains(), timespan_t::from_seconds( data().effectN( 1 ).base_value() ) );
    timespan_t dot_tick_time = dot->current_action->tick_time( state );
    double ticks_left        = remaining / dot_tick_time;

    double total_damage = ticks_left * tick_base_damage;

    action_state_t::release( state );
    this->base_dd_min = this->base_dd_max = total_damage;

    destruction_spell_t::execute();
    td->dots_immolate->adjust_duration( -remaining );
  }
};

struct shadowburn_t : public destruction_spell_t
{
  shadowburn_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "shadowburn", p, p->talents.shadowburn )
  {
    parse_options( options_str );
    can_havoc = true;
  }

  void init() override
  {
    destruction_spell_t::init();

    cooldown->hasted = true;
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      td( s->target )->debuffs_shadowburn->trigger();
    }
  }

  void execute() override
  {
    int shards_used = as<int>( cost() );
    destruction_spell_t::execute();

    if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T28, B2 ) )
    {
      // Shadowburn is an offensive spell that consumes Soul Shards, so it can trigger Impending Ruin
      if ( shards_used > 0 )
      {
        int overflow = p()->buffs.impending_ruin->check() + shards_used - p()->buffs.impending_ruin->max_stack();
        p()->buffs.impending_ruin->trigger( shards_used ); //Stack change callback should switch Impending Ruin to Ritual of Ruin if max stacks reached
        if ( overflow > 0 )
          make_event( sim, 1_ms, [ this, overflow ] { p()->buffs.impending_ruin->trigger( overflow ); } );
      }
    }
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double m = destruction_spell_t::composite_target_crit_chance( target );

    // TOCHECK - Currently no spelldata for the health threshold 08-20-2020
    if ( target->health_percentage() <= 20 )
      m += p()->talents.shadowburn->effectN( 3 ).percent();

    return m;
  }
};

struct dark_soul_instability_t : public destruction_spell_t
{
  dark_soul_instability_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "dark_soul_instability", p, p->talents.dark_soul_instability )
  {
    parse_options( options_str );
    harmful = may_crit = may_miss = false;
  }

  void execute() override
  {
    destruction_spell_t::execute();

    p()->buffs.dark_soul_instability->trigger();
  }
};

// Spells
struct havoc_t : public destruction_spell_t
{
  havoc_t( warlock_t* p, util::string_view options_str ) : destruction_spell_t( p, "Havoc" )
  {
    parse_options( options_str );
    may_crit = false;
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    td( s->target )->debuffs_havoc->trigger();

    // SL - Legendary
    if ( p()->legendary.odr_shawl_of_the_ymirjar->ok() )
      td( s->target )->debuffs_odr->trigger();
  }
};

struct immolate_t : public destruction_spell_t
{
  immolate_t( warlock_t* p, util::string_view options_str ) : destruction_spell_t( "immolate", p, p->find_spell( 348 ) )
  {
    parse_options( options_str );
    const spell_data_t* dmg_spell = player->find_spell( 157736 );

    can_havoc = true;

    //TODO: Check immolate interactions with destro mastery

    // All of the DoT data for Immolate is in spell 157736
    base_tick_time       = dmg_spell->effectN( 1 ).period();
    dot_duration         = dmg_spell->duration();
    spell_power_mod.tick = dmg_spell->effectN( 1 ).sp_coeff();
    hasted_ticks         = true;
    tick_may_crit        = true;
    affected_by_woc      = true;
  }

  void tick( dot_t* d ) override
  {
    destruction_spell_t::tick( d );

    if ( d->state->result == RESULT_CRIT && rng().roll( data().effectN( 2 ).percent() ) )
      p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.immolate_crits );

    p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.immolate );
  }

  void last_tick( dot_t* d ) override
  {
    destruction_spell_t::last_tick( d );

    td( d->target )->debuffs_combusting_engine->expire();
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    td( s->target )->debuffs_combusting_engine->expire();
  }

  double composite_ta_multiplier( const action_state_t* s ) const override 
  {
    double m = destruction_spell_t::composite_ta_multiplier( s );

    if ( td( s->target )->debuffs_combusting_engine->check() )
      m *= 1.0 + td( s->target )->debuffs_combusting_engine->check_stack_value();

    return m;
  }
};

struct conflagrate_t : public destruction_spell_t
{
  timespan_t total_duration;
  timespan_t base_duration;

  conflagrate_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "Conflagrate", p, p->find_spell( 17962 ) ), total_duration(), base_duration()
  {
    parse_options( options_str );
    can_havoc = true;

    energize_type     = action_energize::PER_HIT;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount   = ( p->find_spell( 245330 )->effectN( 1 ).base_value() ) / 10.0;

    cooldown->charges += as<int>( p->spec.conflagrate_2->effectN( 1 ).base_value() );

    if ( p->legendary.cinders_of_the_azjaqir->ok() )
    {
      cooldown->charges += as<int>( p->legendary.cinders_of_the_azjaqir->effectN( 1 ).base_value() );
      cooldown->duration += p->legendary.cinders_of_the_azjaqir->effectN( 2 ).time_value();
    }
  }

  void init() override
  {
    destruction_spell_t::init();

    cooldown->hasted = true;
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    if ( p()->talents.roaring_blaze->ok() && result_is_hit( s->result ) )
      td( s->target )->debuffs_roaring_blaze->trigger();

    //TODO: Check if combusting engine stacks up when there is no immolate on the target (currently implemented as NO)
    if ( p()->conduit.combusting_engine.value() > 0 && result_is_hit( s->result ) && td( s->target )->dots_immolate->is_ticking() )
      td( s->target )->debuffs_combusting_engine->increment( 1, td( s->target)->debuffs_combusting_engine->default_value );
  }

  void execute() override
  {
    destruction_spell_t::execute();

    p()->buffs.backdraft->trigger(
        as<int>( 1 + ( p()->talents.flashover->ok() ? p()->talents.flashover->effectN( 1 ).base_value() : 0 ) ) );

    sim->print_log( "{}: Action {} {} charges remain", player->name(), name(), this->cooldown->current_charge );
  }

  double action_multiplier() const override
  {
    double m = destruction_spell_t::action_multiplier();

    if ( p()->talents.flashover )
    {
      m *= 1.0 + p()->talents.flashover->effectN( 3 ).percent();
    }

    return m;
  }
};

struct incinerate_fnb_t : public destruction_spell_t
{
  double energize_mult;

  incinerate_fnb_t( warlock_t* p ) : destruction_spell_t( "incinerate_fnb", p, p->find_class_spell( "Incinerate" ) )
  {
    aoe        = -1;
    background = true;

    if ( p->talents.fire_and_brimstone->ok() )
    {
      base_multiplier *= p->talents.fire_and_brimstone->effectN( 1 ).percent();
      energize_type     = action_energize::PER_HIT;
      energize_resource = RESOURCE_SOUL_SHARD;
      energize_amount   = ( p->talents.fire_and_brimstone->effectN( 2 ).base_value() ) / 10.0;
      energize_mult = 1.0 + ( p->legendary.embers_of_the_diabolic_raiment->ok() ? p->legendary.embers_of_the_diabolic_raiment->effectN( 1 ).percent() : 0.0 );

      energize_amount *= energize_mult;

      gain = p->gains.incinerate_fnb;
    }
  }

  void init() override
  {
    destruction_spell_t::init();

    // F&B Incinerate's target list depends on the current Havoc target, so it
    // needs to invalidate its target list with the rest of the Havoc spells.
    p()->havoc_spells.push_back( this );
  }

  double cost() const override
  {
    return 0;
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    destruction_spell_t::available_targets( tl );

    // Does not hit the main target
    auto it = range::find( tl, target );
    if ( it != tl.end() )
    {
      tl.erase( it );
    }

    // nor the havoced target
    it = range::find( tl, p()->havoc_target );
    if ( it != tl.end() )
    {
      tl.erase( it );
    }

    return tl.size();
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    if ( s->result == RESULT_CRIT )
      p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1 * energize_mult, p()->gains.incinerate_fnb_crits );
  }

  double composite_crit_chance() const override
  {
    double c = destruction_spell_t::composite_crit_chance();

    if ( p()->buffs.shard_of_annihilation->check() )
      c += p()->buffs.shard_of_annihilation->data().effectN( 1 ).percent();

    return c;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double m = destruction_spell_t::composite_crit_damage_bonus_multiplier();

    if ( p()->buffs.shard_of_annihilation->check() )
      m += p()->buffs.shard_of_annihilation->data().effectN( 2 ).percent();

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = destruction_spell_t::composite_target_multiplier( t );

    auto td = this->td( t );

    // SL - Conduit
    // TOCHECK - Couldn't find affected_by spelldata to reference the spells 08-24-2020.
    if ( td->dots_immolate->is_ticking() && p()->conduit.ashen_remains->ok() )
      m *= 1.0 + p()->conduit.ashen_remains.percent();

    return m;
  }

  double action_multiplier() const override
  {
    double m = destruction_spell_t::action_multiplier();

    m *= 1.0 + p()->buffs.decimating_bolt->check_value();

    return m;
  }
};

struct incinerate_t : public destruction_spell_t
{
  double backdraft_gcd_mult;
  double backdraft_cast_mult;
  double energize_mult;
  incinerate_fnb_t* fnb_action;

  incinerate_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( p, "Incinerate" ), fnb_action( new incinerate_fnb_t( p ) )
  {
    parse_options( options_str );

    add_child( fnb_action );

    can_havoc = true;

    backdraft_cast_mult = 1.0 + p->buffs.backdraft->data().effectN( 1 ).percent();
    backdraft_gcd_mult = 1.0 + p->buffs.backdraft->data().effectN( 2 ).percent();

    energize_type     = action_energize::PER_HIT;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount   = ( p->find_spell( 244670 )->effectN( 1 ).base_value() ) / 10.0;
    energize_mult     = 1.0 + ( p->legendary.embers_of_the_diabolic_raiment->ok() ? p->legendary.embers_of_the_diabolic_raiment->effectN( 1 ).percent() : 0.0 );

    energize_amount *= energize_mult;
  }

  timespan_t execute_time() const override
  {
    timespan_t h = spell_t::execute_time();

    if ( p()->buffs.backdraft->check() )
      h *= backdraft_cast_mult;

    return h;
  }

  timespan_t gcd() const override
  {
    timespan_t t = action_t::gcd();

    if ( t == 0_ms )
      return t;

    if ( p()->buffs.backdraft->check() )
      t *= backdraft_gcd_mult;

    if ( t < min_gcd )
      t = min_gcd;

    return t;
  }

  void execute() override
  {
    destruction_spell_t::execute();

    p()->buffs.backdraft->decrement();

    if ( p()->talents.fire_and_brimstone->ok() )
    {
      fnb_action->set_target( target );
      fnb_action->execute();
    }
    p()->buffs.decimating_bolt->decrement();
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    if ( p()->legendary.shard_of_annihilation.ok() )
      p()->buffs.shard_of_annihilation->decrement();

    //As of 9.0.5, critical strike gains should also be increased by Embers of the Diabolic Raiment. Checked on PTR 2021-03-07
    if ( s->result == RESULT_CRIT )
      p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1 * energize_mult, p()->gains.incinerate_crits );
  }

  double composite_crit_chance() const override
  {
    double c = destruction_spell_t::composite_crit_chance();

    if ( p()->buffs.shard_of_annihilation->check() )
      c += p()->buffs.shard_of_annihilation->data().effectN( 1 ).percent();
    
    return c;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double m = destruction_spell_t::composite_crit_damage_bonus_multiplier();

    if ( p()->buffs.shard_of_annihilation->check() )
      m += p()->buffs.shard_of_annihilation->data().effectN( 2 ).percent();

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = destruction_spell_t::composite_target_multiplier( t );

    auto td = this->td( t );

    // SL - Conduit
    if ( td->dots_immolate->is_ticking() && p()->conduit.ashen_remains->ok() )
      m *= 1.0 + p()->conduit.ashen_remains.percent();

    return m;
  }

  double action_multiplier() const override
  {
    double m = destruction_spell_t::action_multiplier();

    m *= 1.0 + p()->buffs.decimating_bolt->check_value();

    return m;
  }

};



struct chaos_bolt_t : public destruction_spell_t
{
  double backdraft_gcd;
  double backdraft_cast_time;
  double refund;
  internal_combustion_t* internal_combustion;

  chaos_bolt_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( p, "Chaos Bolt" ), refund( 0 ), internal_combustion( new internal_combustion_t( p ) )
  {
    parse_options( options_str );
    can_havoc = true;

    backdraft_cast_time = 1.0 + p->buffs.backdraft->data().effectN( 1 ).percent();
    backdraft_gcd       = 1.0 + p->buffs.backdraft->data().effectN( 2 ).percent();

    add_child( internal_combustion );
  }

  double cost() const override
  {
    double c = destruction_spell_t::cost();

    if ( p()->buffs.ritual_of_ruin->check() )
      c *= 1 + p()->buffs.ritual_of_ruin->data().effectN( 2 ).percent();

    return c;      
  }

  void schedule_execute( action_state_t* state = nullptr ) override
  {
    destruction_spell_t::schedule_execute( state );
  }

  timespan_t execute_time() const override
  {
    timespan_t h = destruction_spell_t::execute_time();
    
    if ( p()->buffs.ritual_of_ruin->check() )
      h *= 1.0 + p()->buffs.ritual_of_ruin->data().effectN( 3 ).percent();

    if ( p()->buffs.backdraft->check() )
      h *= backdraft_cast_time;

    // SL - Legendary
    if ( p()->buffs.madness_of_the_azjaqir->check() )
      h *= 1.0 + p()->buffs.madness_of_the_azjaqir->data().effectN( 2 ).percent();

    return h;
  }

  double action_multiplier() const override
  {
    double m = destruction_spell_t::action_multiplier();

    // SL - Legendary
    if ( p()->buffs.madness_of_the_azjaqir->check() )
    {
      m *= 1.0 + p()->buffs.madness_of_the_azjaqir->data().effectN( 1 ).percent();
    }

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = destruction_spell_t::composite_target_multiplier( t );

    auto td = this->td( t );

    // SL - Conduit
    // TOCHECK - Couldn't find affected_by spelldata to reference the spells 08-24-2020.
    if ( td->dots_immolate->is_ticking() && p()->conduit.ashen_remains->ok() )
      m *= 1.0 + p()->conduit.ashen_remains.percent();

    return m;
  }

  timespan_t gcd() const override
  {
    timespan_t t = destruction_spell_t::gcd();

    if ( t == 0_ms )
      return t;

    // PTR 2022-02-16: Backdraft is no longer consumed when using T28 free Chaos Bolt cast, but GCD is still shortened
    if ( p()->buffs.backdraft->check() )
      t *= backdraft_gcd;

    if ( t < min_gcd )
      t = min_gcd;

    return t;
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    trigger_internal_combustion( s );

    if ( p()->talents.eradication->ok() && result_is_hit( s->result ) )
      td( s->target )->debuffs_eradication->trigger();
  }

  void trigger_internal_combustion( action_state_t* s )
  {
    if ( !p()->talents.internal_combustion->ok() )
      return;

    if ( !result_is_hit( s->result ) )
      return;

    auto td = this->td( s->target );
    if ( !td->dots_immolate->is_ticking() )
      return;

    internal_combustion->set_target( s->target );
    internal_combustion->execute();
  }

  void execute() override
  {
    int shards_used = as<int>( cost() );
    destruction_spell_t::execute();

    // PTR 2022-02-16: Backdraft is no longer consumed for T28 free Chaos Bolts
    if ( !p()->buffs.ritual_of_ruin->check() )
      p()->buffs.backdraft->decrement();

    // SL - Legendary
    if ( p()->legendary.madness_of_the_azjaqir->ok() )
      p()->buffs.madness_of_the_azjaqir->trigger();

    if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T28, B2 ) )
    {
      if ( p()->buffs.ritual_of_ruin->check() )
      {
        if (p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T28, B4 ))
        {
          // Note: Tier set spell (363950) has duration in Effect 1, but there is also a duration adjustment in Ritual of Ruin buff data Effect 4
          // Unsure which is being used at this time
          timespan_t duration = p()->sets->set( WARLOCK_DESTRUCTION, T28, B4 )->effectN( 1 ).time_value() * 1000;
          if ( p()->warlock_pet_list.blasphemy.active_pet() )
          {
            p()->warlock_pet_list.blasphemy.active_pet()->adjust_duration( duration );
          }
          else
          {
            p()->warlock_pet_list.blasphemy.spawn( duration, 1U );
          }

          if ( p()->talents.rain_of_chaos->ok() )
          {
            p()->buffs.rain_of_chaos->extend_duration_or_trigger( duration );
          }

          // TOFIX: As of 2022-02-03 PTR, Blasphemy appears to trigger Infernal Awakening on spawn, and Blasphemous Existence if already out
          // This will require first fixing Infernal Awakening to properly be on Infernal pet
          p()->warlock_pet_list.blasphemy.active_pet()->blasphemous_existence->execute();
          p()->procs.avatar_of_destruction->occur();
        }

        p()->buffs.ritual_of_ruin->expire();
      }

      // Any changes to Impending Ruin must also be made in rain_of_fire_t!
      if ( shards_used > 0 )
      {
        int overflow = p()->buffs.impending_ruin->check() + shards_used - p()->buffs.impending_ruin->max_stack();
        p()->buffs.impending_ruin->trigger( shards_used ); //Stack change callback should switch Impending Ruin to Ritual of Ruin if max stacks reached
        if ( overflow > 0 )
          make_event( sim, 1_ms, [ this, overflow ] { p()->buffs.impending_ruin->trigger( overflow ); } );
      }
    }
  }

  // Force spell to always crit
  double composite_crit_chance() const override
  {
    return 1.0;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    destruction_spell_t::calculate_direct_amount( state );

    // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
    // player spell crit instead. The state target crit chance of the state object is correct.
    // Targeted Crit debuffs function as a separate multiplier.
    // Updated 06-24-2019: Target crit chance appears to no longer increase Chaos Bolt damage.
    state->result_total *= 1.0 + player->cache.spell_crit_chance();  //+ state->target_crit_chance;

    return state->result_total;
  }
};

struct infernal_awakening_t : public destruction_spell_t
{
  infernal_awakening_t( warlock_t* p ) : destruction_spell_t( "infernal_awakening", p, p->find_spell( 22703 ) )
  {
    destro_mastery = false;
    aoe            = -1;
    background     = true;
    dual           = true;
    trigger_gcd    = 0_ms;
    base_multiplier *= 1.0 + p->spec.summon_infernal_2->effectN( 1 ).percent();
  }
};

struct summon_infernal_t : public destruction_spell_t
{
  infernal_awakening_t* infernal_awakening;
  timespan_t infernal_duration;

  summon_infernal_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "summon_infernal", p, p->find_spell( 1122 ) ), infernal_awakening( nullptr )
  {
    parse_options( options_str );

    harmful = may_crit        = false;
    infernal_duration         = p->find_spell( 111685 )->duration() + 1_ms;
    infernal_awakening        = new infernal_awakening_t( p );
    infernal_awakening->stats = stats;
    radius                    = infernal_awakening->radius;
  }

  void execute() override
  {
    destruction_spell_t::execute();

    // TODO - Make infernal not spawn until after infernal awakening impacts.
    if ( infernal_awakening )
      infernal_awakening->execute();

    for ( auto* infernal : p()->warlock_pet_list.infernals )
    {
      if ( infernal->is_sleeping() )
      {
        infernal->summon( infernal_duration );
      }
    }

    if ( p()->talents.rain_of_chaos->ok() )
    {
      p()->buffs.rain_of_chaos->extend_duration_or_trigger();
    }
  }

  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( data().missile_speed() );
  }
};

// AOE Spells
struct rain_of_fire_t : public destruction_spell_t
{
  struct rain_of_fire_tick_t : public destruction_spell_t
  {
    rain_of_fire_tick_t( warlock_t* p ) : destruction_spell_t( "rain_of_fire_tick", p, p->find_spell( 42223 ) )
    {
      aoe        = -1;
      background = dual = direct_tick = true;  // Legion TOCHECK
      callbacks                       = false;
      radius                          = p->find_spell( 5740 )->effectN( 1 ).radius();
      base_multiplier *= 1.0 + p->spec.rain_of_fire_2->effectN( 1 ).percent();
      base_multiplier *= 1.0 + p->talents.inferno->effectN( 2 ).percent();
    }

    void impact( action_state_t* s ) override
    {
      destruction_spell_t::impact( s );

      if ( p()->talents.inferno && result_is_hit( s->result ) )
      {
        if ( rng().roll( p()->talents.inferno->effectN( 1 ).percent() ) )
        {
          p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.inferno );
        }
      }
    }

    void execute() override
    {
      destruction_spell_t::execute();
    }
  };

  rain_of_fire_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "rain_of_fire", p, p->find_spell( 5740 ) )
  {
    parse_options( options_str );
    aoe          = -1;
    dot_duration = 0_ms;
    may_miss = may_crit = false;
    base_tick_time      = data().duration() / 8.0;  // ticks 8 times (missing from spell data)
    base_execute_time   = 0_ms;                     // HOTFIX

    if ( !p->active.rain_of_fire )
    {
      p->active.rain_of_fire        = new rain_of_fire_tick_t( p );
      p->active.rain_of_fire->stats = stats;
    }
  }

  double cost() const override
  {
    double c = destruction_spell_t::cost();

    if ( p()->buffs.ritual_of_ruin->check() )
      c *= 1 + p()->buffs.ritual_of_ruin->data().effectN( 2 ).percent();

    return c;        
  }

  void execute() override
  {
    int shards_used = as<int>( cost() );
    destruction_spell_t::execute();

    if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T28, B2 ) )
    {
      if ( p()->buffs.ritual_of_ruin->check() )
      {
        if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T28, B4 ) )
        {
          // Note: Tier set spell (363950) has duration in Effect 1, but there is also a duration adjustment in Ritual of Ruin buff data Effect 4
          // Unsure which is being used at this time
          timespan_t duration = p()->sets->set( WARLOCK_DESTRUCTION, T28, B4 )->effectN( 1 ).time_value() * 1000;
          if ( p()->warlock_pet_list.blasphemy.active_pet() )
          {
            p()->warlock_pet_list.blasphemy.active_pet()->adjust_duration( duration );
          }
          else
          {
            p()->warlock_pet_list.blasphemy.spawn( duration, 1U );
          }

          if ( p()->talents.rain_of_chaos->ok() )
          {
            p()->buffs.rain_of_chaos->extend_duration_or_trigger( duration );
          }

          // TOFIX: As of 2022-02-03 PTR, Blasphemy appears to trigger Infernal Awakening on spawn, and Blasphemous Existence if already out
          // This will require first fixing Infernal Awakening to properly be on Infernal pet
          p()->warlock_pet_list.blasphemy.active_pet()->blasphemous_existence->execute();
          p()->procs.avatar_of_destruction->occur();
        }
        p()->buffs.ritual_of_ruin->expire();
      }

      // Any changes to Impending Ruin must also be made in chaos_bolt_t!
      if ( shards_used > 0 )
      {
        int overflow = p()->buffs.impending_ruin->check() + shards_used - p()->buffs.impending_ruin->max_stack();
        p()->buffs.impending_ruin->trigger( shards_used ); //Stack change callback should switch Impending Ruin to Ritual of Ruin if max stacks reached
        if ( overflow > 0 )
          make_event( sim, 1_ms, [ this, overflow ] { p()->buffs.impending_ruin->trigger( overflow ); } );
      }
    }

    make_event<ground_aoe_event_t>( *sim, p(),
                                    ground_aoe_params_t()
                                        .target( execute_state->target )
                                        .x( execute_state->target->x_position )
                                        .y( execute_state->target->y_position )
                                        .pulse_time( base_tick_time * player->cache.spell_haste() )
                                        .duration( data().duration() * player->cache.spell_haste() )
                                        .start_time( sim->current_time() )
                                        .action( p()->active.rain_of_fire ) );
  }
};

// Talents which need initialization after baseline spells
struct channel_demonfire_tick_t : public destruction_spell_t
{
  channel_demonfire_tick_t( warlock_t* p ) : destruction_spell_t( "channel_demonfire_tick", p, p->find_spell( 196448 ) )
  {
    background = true;
    may_miss   = false;
    dual       = true;

    spell_power_mod.direct = data().effectN( 1 ).sp_coeff();

    aoe                 = -1;
    base_aoe_multiplier = data().effectN( 2 ).sp_coeff() / data().effectN( 1 ).sp_coeff();
  }
};

struct channel_demonfire_t : public destruction_spell_t
{
  channel_demonfire_tick_t* channel_demonfire;
  int immolate_action_id;

  channel_demonfire_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "channel_demonfire", p, p->talents.channel_demonfire ),
      channel_demonfire( new channel_demonfire_tick_t( p ) ),
      immolate_action_id( 0 )
  {
    parse_options( options_str );
    channeled    = true;
    hasted_ticks = true;
    may_crit     = false;

    add_child( channel_demonfire );
  }

  void init() override
  {
    destruction_spell_t::init();

    cooldown->hasted   = true;
    immolate_action_id = p()->find_action_id( "immolate" );
  }

  // TODO: This is suboptimal, can this be changed to available_targets() in some way?
  std::vector<player_t*>& target_list() const override
  {
    target_cache.list = destruction_spell_t::target_list();

    size_t i = target_cache.list.size();
    while ( i > 0 )
    {
      i--;
      player_t* current_target = target_cache.list[ i ];

      auto td = this->td( current_target );
      if ( !td->dots_immolate->is_ticking() )
        target_cache.list.erase( target_cache.list.begin() + i );
    }
    return target_cache.list;
  }

  void tick( dot_t* d ) override
  {
    // Need to invalidate the target cache to figure out immolated targets.
    target_cache.is_valid = false;

    const auto& targets = target_list();

    if ( !targets.empty() )
    {
      channel_demonfire->set_target( targets[ rng().range( size_t(), targets.size() ) ] );
      channel_demonfire->execute();
    }

    destruction_spell_t::tick( d );
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return s->action->tick_time( s ) * 15.0;
  }

  bool ready() override
  {
    double active_immolates = p()->get_active_dots( immolate_action_id );

    if ( active_immolates == 0 )
      return false;

    return destruction_spell_t::ready();
  }
};

struct soul_fire_t : public destruction_spell_t
{
  immolate_t* immolate;

  soul_fire_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "soul_fire", p, p->talents.soul_fire ), immolate( new immolate_t( p, "" ) )
  {
    parse_options( options_str );
    energize_type     = action_energize::PER_HIT;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount   = ( p->find_spell( 281490 )->effectN( 1 ).base_value() ) / 10.0;

    can_havoc = true;

    immolate->background                  = true;
    immolate->dual                        = true;
    immolate->base_costs[ RESOURCE_MANA ] = 0;
    immolate->base_dd_multiplier *= 0.0;
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      immolate->set_target( s->target );
      immolate->execute();
    }
  }
};

struct cataclysm_t : public destruction_spell_t
{
  immolate_t* immolate;

  cataclysm_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "cataclysm", p, p->talents.cataclysm ), immolate( new immolate_t( p, "" ) )
  {
    parse_options( options_str );
    aoe = -1;

    immolate->background                  = true;
    immolate->dual                        = true;
    immolate->base_costs[ RESOURCE_MANA ] = 0;
    immolate->base_dd_multiplier *= 0.0;
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      immolate->set_target( s->target );
      immolate->execute();
    }
  }
};
}  // namespace actions_destruction

namespace buffs
{
}  // namespace buffs

// add actions
action_t* warlock_t::create_action_destruction( util::string_view action_name, util::string_view options_str )
{
  using namespace actions_destruction;

  if ( action_name == "conflagrate" )
    return new conflagrate_t( this, options_str );
  if ( action_name == "incinerate" )
    return new incinerate_t( this, options_str );
  if ( action_name == "immolate" )
    return new immolate_t( this, options_str );
  if ( action_name == "chaos_bolt" )
    return new chaos_bolt_t( this, options_str );
  if ( action_name == "rain_of_fire" )
    return new rain_of_fire_t( this, options_str );
  if ( action_name == "havoc" )
    return new havoc_t( this, options_str );
  if ( action_name == "summon_infernal" )
    return new summon_infernal_t( this, options_str );

  // Talents
  if ( action_name == "soul_fire" )
    return new soul_fire_t( this, options_str );
  if ( action_name == "shadowburn" )
    return new shadowburn_t( this, options_str );
  if ( action_name == "cataclysm" )
    return new cataclysm_t( this, options_str );
  if ( action_name == "channel_demonfire" )
    return new channel_demonfire_t( this, options_str );
  if ( action_name == "dark_soul_instability" )
    return new dark_soul_instability_t( this, options_str );

  return nullptr;
}
void warlock_t::create_buffs_destruction()
{
  // destruction buffs
  buffs.backdraft =
      make_buff( this, "backdraft", find_spell( 117828 ) )
          ->set_refresh_behavior( buff_refresh_behavior::DURATION )
          ->set_max_stack( as<int>( find_spell( 117828 )->max_stacks() +
                                    ( talents.flashover ? talents.flashover->effectN( 2 ).base_value() : 0 ) ) );

  buffs.reverse_entropy = make_buff( this, "reverse_entropy", talents.reverse_entropy )
                              ->set_default_value( find_spell( 266030 )->effectN( 1 ).percent() )
                              ->set_duration( find_spell( 266030 )->duration() )
                              ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                              ->set_trigger_spell( talents.reverse_entropy )
                              ->add_invalidate( CACHE_HASTE );

  // Spell 335236 holds the duration of the proc'd infernal's duration, storing it in default value of the buff for use
  // later
  buffs.rain_of_chaos = make_buff( this, "rain_of_chaos", find_spell( 266087 ) )
                            ->set_default_value( find_spell( 335236 )->_duration );

  buffs.dark_soul_instability = make_buff( this, "dark_soul_instability", talents.dark_soul_instability )
                                    ->add_invalidate( CACHE_SPELL_CRIT_CHANCE )
                                    ->add_invalidate( CACHE_CRIT_CHANCE )
                                    ->set_default_value( talents.dark_soul_instability->effectN( 1 ).percent() );

  // Legendaries
  buffs.madness_of_the_azjaqir =
      make_buff( this, "madness_of_the_azjaqir", legendary.madness_of_the_azjaqir->effectN( 1 ).trigger() )
          ->set_trigger_spell( legendary.madness_of_the_azjaqir );

  // Tier Sets
  buffs.impending_ruin = make_buff ( this, "impending_ruin", find_spell ( 364348 ) )
                             ->set_stack_change_callback( [ this ]( buff_t* b, int, int cur )
                               {
                                 if ( cur == b->max_stack() )
                                 {
                                   make_event( sim, 0_ms, [ this, b ] { 
                                     buffs.ritual_of_ruin->trigger();
                                     b->expire();
                                     });
                                 };
                               });

  buffs.ritual_of_ruin = make_buff ( this, "ritual_of_ruin", find_spell ( 364349 ) );
}
void warlock_t::init_spells_destruction()
{
  using namespace actions_destruction;

  spec.destruction                = find_specialization_spell( 137046 );
  mastery_spells.chaotic_energies = find_mastery_spell( WARLOCK_DESTRUCTION );

  spec.conflagrate       = find_specialization_spell( "Conflagrate" );
  spec.conflagrate_2     = find_specialization_spell( 231793 );
  spec.havoc             = find_specialization_spell( "Havoc" );
  spec.havoc_2           = find_specialization_spell( 335174 );
  spec.rain_of_fire_2    = find_specialization_spell( 335189 );
  spec.summon_infernal_2 = find_specialization_spell( 335175 );

  // Talents
  talents.flashover   = find_talent_spell( "Flashover" );
  talents.eradication = find_talent_spell( "Eradication" );
  talents.soul_fire   = find_talent_spell( "Soul Fire" );

  talents.reverse_entropy     = find_talent_spell( "Reverse Entropy" );
  talents.internal_combustion = find_talent_spell( "Internal Combustion" );
  talents.shadowburn          = find_talent_spell( "Shadowburn" );

  talents.inferno            = find_talent_spell( "Inferno" );
  talents.fire_and_brimstone = find_talent_spell( "Fire and Brimstone" );
  talents.cataclysm          = find_talent_spell( "Cataclysm" );

  talents.roaring_blaze = find_talent_spell( "Roaring Blaze" );
  talents.rain_of_chaos = find_talent_spell( "Rain of Chaos" );

  talents.channel_demonfire     = find_talent_spell( "Channel Demonfire" );
  talents.dark_soul_instability = find_talent_spell( "Dark Soul: Instability" );

  // Legendaries
  legendary.cinders_of_the_azjaqir         = find_runeforge_legendary( "Cinders of the Azj'Aqir" );
  legendary.embers_of_the_diabolic_raiment = find_runeforge_legendary( "Embers of the Diabolic Raiment" );
  legendary.madness_of_the_azjaqir         = find_runeforge_legendary( "Madness of the Azj'Aqir" );
  legendary.odr_shawl_of_the_ymirjar       = find_runeforge_legendary( "Odr, Shawl of the Ymirjar" );

  // Conduits
  conduit.ashen_remains     = find_conduit_spell( "Ashen Remains" );
  conduit.combusting_engine = find_conduit_spell( "Combusting Engine" );
  conduit.infernal_brand    = find_conduit_spell( "Infernal Brand" );
  //conduit.duplicitous_havoc is done in main module
}

void warlock_t::init_gains_destruction()
{
  gains.conflagrate          = get_gain( "conflagrate" );
  gains.immolate             = get_gain( "immolate" );
  gains.immolate_crits       = get_gain( "immolate_crits" );
  gains.incinerate           = get_gain( "incinerate" );
  gains.incinerate_crits     = get_gain( "incinerate_crits" );
  gains.incinerate_fnb       = get_gain( "incinerate_fnb" );
  gains.incinerate_fnb_crits = get_gain( "incinerate_fnb_crits" );
  gains.soul_fire            = get_gain( "soul_fire" );
  gains.infernal             = get_gain( "infernal" );
  gains.shadowburn_refund    = get_gain( "shadowburn_refund" );
  gains.inferno              = get_gain( "inferno" );
}

void warlock_t::init_rng_destruction()
{
  //TOCHECK: SPELL DATA LISTS % CHANCE BUT THIS IS SUPPOSEDLY DECK OF CARDS
  //2020-10-12: PTR for prepatch says 15% but beta still says 20%. 20% version was supposedly 2 out of 10
  //PTR version is untested, using 3 out of 20 for now. Hopefully there is a way to tie this to spell data later.
  rain_of_chaos_rng = get_shuffled_rng( "rain_of_chaos", 3, 20 );
}

void warlock_t::init_procs_destruction()
{
  procs.reverse_entropy = get_proc( "reverse_entropy" );
  procs.rain_of_chaos = get_proc( "rain_of_chaos" );
}

void warlock_t::create_options_destruction()
{
}

//destruction_apl_start
void warlock_t::create_apl_destruction()
{
  action_priority_list_t* def = get_action_priority_list( "default" );
  action_priority_list_t* aoe = get_action_priority_list( "aoe" );
  action_priority_list_t* cds = get_action_priority_list( "cds" );
  action_priority_list_t* havoc = get_action_priority_list( "havoc" );

  def->add_action( "call_action_list,name=havoc,if=havoc_active&active_enemies>1&active_enemies<5-talent.inferno.enabled+(talent.inferno.enabled&talent.internal_combustion.enabled)" );
  def->add_action( "fleshcraft,if=soulbind.volatile_solvent,cancel_if=buff.volatile_solvent_humanoid.up" );
  def->add_action( "conflagrate,if=talent.roaring_blaze.enabled&debuff.roaring_blaze.remains<1.5" );
  def->add_action( "cataclysm" );
  def->add_action( "call_action_list,name=aoe,if=active_enemies>2-set_bonus.tier28_4pc" );
  def->add_action( "soul_fire,cycle_targets=1,if=refreshable&soul_shard<=4&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>remains)" );
  def->add_action( "immolate,cycle_targets=1,if=remains<3&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>remains)" );
  def->add_action( "immolate,if=talent.internal_combustion.enabled&action.chaos_bolt.in_flight&remains<duration*0.5" );
  def->add_action( "chaos_bolt,if=(pet.infernal.active|pet.blasphemy.active)&soul_shard>=4" );
  def->add_action( "call_action_list,name=cds" );
  def->add_action( "channel_demonfire" );
  def->add_action( "scouring_tithe" );
  def->add_action( "decimating_bolt" );
  def->add_action( "havoc,cycle_targets=1,if=!(target=self.target)&(dot.immolate.remains>dot.immolate.duration*0.5|!talent.internal_combustion.enabled)" );
  def->add_action( "impending_catastrophe" );
  def->add_action( "soul_rot" );
  def->add_action( "havoc,if=runeforge.odr_shawl_of_the_ymirjar.equipped" );
  def->add_action( "variable,name=pool_soul_shards,value=active_enemies>1&cooldown.havoc.remains<=10|buff.ritual_of_ruin.up&talent.rain_of_chaos" );
  def->add_action( "conflagrate,if=buff.backdraft.down&soul_shard>=1.5-0.3*talent.flashover.enabled&!variable.pool_soul_shards" );
  def->add_action( "chaos_bolt,if=pet.infernal.active|buff.rain_of_chaos.remains>cast_time" );
  def->add_action( "chaos_bolt,if=buff.backdraft.up&!variable.pool_soul_shards" );
  def->add_action( "chaos_bolt,if=talent.eradication&!variable.pool_soul_shards&debuff.eradication.remains<cast_time" );
  def->add_action( "shadowburn,if=!variable.pool_soul_shards|soul_shard>=4.5" );
  def->add_action( "chaos_bolt,if=soul_shard>3.5" );
  def->add_action( "chaos_bolt,if=time_to_die<5&time_to_die>cast_time+travel_time" );
  def->add_action( "conflagrate,if=charges>1|time_to_die<gcd" );
  def->add_action( "incinerate" );

  aoe->add_action( "rain_of_fire,if=pet.infernal.active&(!cooldown.havoc.ready|active_enemies>3)" );
  aoe->add_action( "rain_of_fire,if=set_bonus.tier28_4pc" );
  aoe->add_action( "soul_rot" );
  aoe->add_action( "impending_catastrophe" );
  aoe->add_action( "channel_demonfire,if=dot.immolate.remains>cast_time" );
  aoe->add_action( "immolate,cycle_targets=1,if=active_enemies<5&remains<5&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>remains)" );
  aoe->add_action( "call_action_list,name=cds" );
  aoe->add_action( "havoc,cycle_targets=1,if=!(target=self.target)&active_enemies<4" );
  aoe->add_action( "rain_of_fire" );
  aoe->add_action( "havoc,cycle_targets=1,if=!(self.target=target)" );
  aoe->add_action( "decimating_bolt" );
  aoe->add_action( "incinerate,if=talent.fire_and_brimstone.enabled&buff.backdraft.up&soul_shard<5-0.2*active_enemies" );
  aoe->add_action( "soul_fire" );
  aoe->add_action( "conflagrate,if=buff.backdraft.down" );
  aoe->add_action( "shadowburn,if=target.health.pct<20" );
  aoe->add_action( "immolate,if=refreshable");
  aoe->add_action( "scouring_tithe" );
  aoe->add_action( "incinerate" );

  cds->add_action( "use_item,name=shadowed_orb_of_torment,if=cooldown.summon_infernal.remains<3|time_to_die<42" );
  cds->add_action( "summon_infernal" );
  cds->add_action( "dark_soul_instability,if=pet.infernal.active|cooldown.summon_infernal.remains_expected>time_to_die" );
  cds->add_action( "potion,if=pet.infernal.active" );
  cds->add_action( "berserking,if=pet.infernal.active" );
  cds->add_action( "blood_fury,if=pet.infernal.active" );
  cds->add_action( "fireblood,if=pet.infernal.active" );
  cds->add_action( "use_item,name=scars_of_fraternal_strife,if=!buff.scars_of_fraternal_strife_4.up" );
  cds->add_action( "use_item,name=scars_of_fraternal_strife,if=buff.scars_of_fraternal_strife_4.up&pet.infernal.active" );
  cds->add_action( "use_items,if=pet.infernal.active|time_to_die<21" );

  havoc->add_action( "conflagrate,if=buff.backdraft.down&soul_shard>=1&soul_shard<=4" );
  havoc->add_action( "soul_fire,if=cast_time<havoc_remains" );
  havoc->add_action( "decimating_bolt,if=cast_time<havoc_remains&soulbind.lead_by_example.enabled" );
  havoc->add_action( "scouring_tithe,if=cast_time<havoc_remains" );
  havoc->add_action( "immolate,if=talent.internal_combustion.enabled&remains<duration*0.5|!talent.internal_combustion.enabled&refreshable" );
  havoc->add_action( "chaos_bolt,if=cast_time<havoc_remains&!(set_bonus.tier28_4pc&active_enemies>1&talent.inferno.enabled)" );
  havoc->add_action( "rain_of_fire,if=set_bonus.tier28_4pc&active_enemies>1&talent.inferno.enabled" );
  havoc->add_action( "shadowburn" );
  havoc->add_action( "incinerate,if=cast_time<havoc_remains" );
}
//destruction_apl_end
}  // namespace warlock
