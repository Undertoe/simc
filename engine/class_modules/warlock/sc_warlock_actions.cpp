#include "simulationcraft.hpp"

#include "sc_warlock.hpp"

#include "sc_warlock_pets.hpp"
#include "util/util.hpp"

namespace warlock
{
using namespace helpers;

  struct warlock_spell_t : public spell_t
  {
    struct affected_by_t
    {
      // Class

      // Affliction
      bool potent_afflictions_td = false;
      bool potent_afflictions_dd = false;
      bool malediction = false;
      bool contagion = false;

      // Demonology
      bool master_demonologist_dd = false;
      bool sacrificed_souls = false;
      bool wicked_maw = false;
      bool soul_conduit_base_cost = false;

      // Destruction
      bool havoc = false;
      bool backdraft = false;
      bool roaring_blaze = false;

    } affected_by;

    struct triggers_t
    {
      // Class

      // Affliction

      // Demonology

      // Destruction

    } triggers;

    warlock_spell_t( util::string_view token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() )
    : spell_t( token, p, s ),
      affected_by(),
      triggers()
    {
      may_crit = true;
      tick_may_crit = true;
      weapon_multiplier = 0.0;

      affected_by.potent_afflictions_td = data().affected_by( p->warlock_base.potent_afflictions->effectN( 1 ) );
      affected_by.potent_afflictions_dd = data().affected_by( p->warlock_base.potent_afflictions->effectN( 2 ) );

      affected_by.master_demonologist_dd = data().affected_by( p->warlock_base.master_demonologist->effectN( 2 ) );
    }

    warlock_spell_t( util::string_view token, warlock_t* p, const spell_data_t* s, util::string_view options_str )
      : warlock_spell_t( token, p, s )
    { parse_options( options_str ); }

    warlock_t* p()
    { return static_cast<warlock_t*>( player ); }
    
    const warlock_t* p() const
    { return static_cast<warlock_t*>( player ); }

    warlock_td_t* td( player_t* t )
    { return p()->get_target_data( t ); }

    const warlock_td_t* td( player_t* t ) const
    { return p()->get_target_data( t ); }

    void reset() override
    { spell_t::reset(); }

    void consume_resource() override
    {
      spell_t::consume_resource();

      if ( resource_current == RESOURCE_SOUL_SHARD && p()->in_combat )
      {
        int shards_used = as<int>( last_resource_cost );
        int base_shards = as<int>( base_cost() ); // Power Overwhelming is ignoring any cost changes

      }
    }

    void execute() override
    {
      spell_t::execute();

    }

    void impact( action_state_t* s ) override
    {
      spell_t::impact( s );

      // if ( affected_by.havoc && p()->talents.mayhem.ok() )
      // {
      //   // Havoc debuff has an ICD, so it is safe to attempt a trigger
      //   auto tl = target_list();
      //   auto n = available_targets( tl );

      //   if ( n > 1u )
      //   {
      //     player_t* trigger_target = tl.at( 1u + rng().range( n - 1u ) );
      //     if ( td( trigger_target )->debuffs_havoc->trigger() )
      //     {
      //       p()->procs.mayhem->occur();
      //     }
      //   }
      // }

    }

    void tick( dot_t* d ) override
    {
      spell_t::tick( d );

      // if ( affliction() && triggers.ravenous_afflictions && d->state->result == RESULT_CRIT && p()->ravenous_afflictions_rng->trigger() )
      // {
      //   p()->buffs.nightfall->trigger();
      //   p()->procs.ravenous_afflictions->occur();
      // }

      {
        int rift = rng().range( 3 );

        switch ( rift )
        {
        case 0:
          p()->warlock_pet_list.shadow_rifts.spawn( p()->talents.shadowy_tear_summon->duration() );
          break;
        case 1:
          p()->warlock_pet_list.unstable_rifts.spawn( p()->talents.unstable_tear_summon->duration() );
          break;
        case 2:
          p()->warlock_pet_list.chaos_rifts.spawn( p()->talents.chaos_tear_summon->duration() );
          break;
        default:
          break;
        }

        p()->procs.dimension_ripper->occur();
      }
    }

    double composite_crit_chance() const override
    {
      double c = spell_t::composite_crit_chance();

      // if ( affliction() && affected_by.malediction )
      //   c += p()->talents.malediction->effectN( 1 ).percent();

      return c;
    }

    double composite_crit_chance_multiplier() const override
    {
      double m = spell_t::composite_crit_chance_multiplier();

      // if ( hellcaller() && affected_by.xalans_ferocity_crit )
      //   m *= 1.0 + p()->hero.xalans_ferocity->effectN( 4 ).percent();

      return m;
    }

    double composite_crit_damage_bonus_multiplier() const override
    {
      double m = spell_t::composite_crit_damage_bonus_multiplier();

      // if ( affliction() && affected_by.contagion )
      //   m *= 1.0 + p()->talents.contagion->effectN( 1 ).percent();

      return m;
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = spell_t::composite_target_multiplier( t );

      // if ( affliction() && affected_by.infirmity )
      //   m *= 1.0 + td( t )->debuffs_infirmity->check_stack_value();

      return m;
    }

    double action_multiplier() const override
    {
      double m = spell_t::action_multiplier();

      // if ( demonology() && affected_by.master_demonologist_dd )
      //   m *= 1.0 + p()->cache.mastery_value();

      return m;
    }

    double composite_persistent_multiplier( const action_state_t* s ) const override
    {
      double m = spell_t::composite_persistent_multiplier( s );

      return m;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = spell_t::composite_da_multiplier( s );

      // if ( affliction() && affected_by.potent_afflictions_dd )
      //   m *= 1.0 + p()->cache.mastery_value();

      return m;
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double m = spell_t::composite_ta_multiplier( s );

      // if ( affliction() && affected_by.potent_afflictions_td )
      //   m *= 1.0 + p()->cache.mastery_value();

      return m;
    }

    double execute_time_pct_multiplier() const override
    {
      double m = spell_t::execute_time_pct_multiplier();

      // if ( destruction() && affected_by.backdraft && p()->buffs.backdraft->check() )
      //   m *= 1.0 + p()->talents.backdraft_buff->effectN( 1 ).percent();


      return m;
    }

    timespan_t gcd() const override
    {
      timespan_t t = spell_t::gcd();

      if ( !destruction() )
        return t;

      if ( t == 0_ms )
        return t;

      // if ( affected_by.backdraft && p()->buffs.backdraft->check() )
      //   t *= 1.0 + p()->talents.backdraft_buff->effectN( 2 ).percent();

      if ( t < min_gcd )
        t = min_gcd;

      return t;
    }

    void extend_dot( dot_t* dot, timespan_t extend_duration )
    {
      if ( dot->is_ticking() )
      {
        // TODO: Do we always cap out at pandemic amount (+50%)?
        dot->adjust_duration( extend_duration, dot->current_action->dot_duration * 1.5 );
      }
    }

    bool use_havoc() const
    {
      // Ensure we do not try to hit the same target twice.
      return affected_by.havoc && p()->havoc_target && p()->havoc_target != target;
    }

    // We need to ensure that the target cache is invalidated, which sometimes does not take 
    // place in select_target() due to other methods we have overridden involving Havoc
    bool select_target() override
    {
      auto saved_target = target;

      bool passed = spell_t::select_target();

      if ( passed && target != saved_target && use_havoc() )
        target_cache.is_valid = false;

      return passed;
    }

    int n_targets() const override
    {
      if ( destruction() && use_havoc() )
      {
        assert( spell_t::n_targets() == 0 );
        return 2;
      }
      else
      {
        return spell_t::n_targets();
      }
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      spell_t::available_targets( tl );

      // Check target list size to prevent some silly scenarios where Havoc target
      // is the only target in the list.
      if ( destruction() && tl.size() > 1 && use_havoc())
      {
        // We need to make sure that the Havoc target ends up second in the target list,
        // so that Havoc spells can pick it up correctly.
        auto it = range::find(tl, p()->havoc_target);
        if (it != tl.end())
        {
          tl.erase(it);
          tl.insert(tl.begin() + 1, p()->havoc_target);
        }
      }

      return tl.size();
    }

    void init() override
    {
      spell_t::init();

      // if ( destruction() && affected_by.havoc )
      // {
      //   base_aoe_multiplier *= p()->talents.havoc_debuff->effectN( 1 ).percent() + p()->hero.gloom_of_nathreza->effectN( 2 ).percent();
      //   p()->havoc_spells.push_back( this );
      // }

    }

    bool affliction() const
    { return p()->specialization() == WARLOCK_AFFLICTION; }

    bool demonology() const
    { return p()->specialization() == WARLOCK_DEMONOLOGY; }

    bool destruction() const
    { return p()->specialization() == WARLOCK_DESTRUCTION; }

    bool active_2pc( set_bonus_type_e tier ) const
    { return p()->sets->has_set_bonus( p()->specialization(), tier, B2 ); }

    bool active_4pc( set_bonus_type_e tier ) const
    { return p()->sets->has_set_bonus( p()->specialization(), tier, B4 ); }
  };

  // Shared Class Actions Begin

  struct summon_pet_t : public warlock_spell_t
  {
    timespan_t summoning_duration;
    std::string pet_name;
    warlock_pet_t* pet;

  private:
    void _init_summon_pet_t()
    {
      util::tokenize( pet_name );
      harmful = false;

      if ( data().ok()
        && std::find( p()->pet_name_list.begin(), p()->pet_name_list.end(), pet_name ) == p()->pet_name_list.end() )
        p()->pet_name_list.push_back( pet_name );

      target = player;
    }

  public:
    summon_pet_t( util::string_view n, warlock_t* p, const spell_data_t* sd )
      : warlock_spell_t( n, p, sd ),
      summoning_duration( 0_ms ),
      pet_name( n ),
      pet( nullptr )
    { _init_summon_pet_t(); }

    summon_pet_t( util::string_view n, warlock_t* p, int id )
      : summon_pet_t( n, p, p->find_spell( id ) )
    { }

    summon_pet_t( util::string_view n, warlock_t* p )
      : summon_pet_t( n, p, p->find_class_spell( fmt::format( "Summon {}", n ) ) )
    { }

    void init_finished() override
    {
      pet = debug_cast<warlock_pet_t*>( player->find_pet( pet_name ) );

      warlock_spell_t::init_finished();
    }

    virtual void execute() override
    {
      pet->summon( summoning_duration );

      warlock_spell_t::execute();
    }

    bool ready() override
    {
      if ( !pet )
        return false;

      return warlock_spell_t::ready();
    }
  };

  struct summon_main_pet_t : public summon_pet_t
  {
    summon_main_pet_t( util::string_view n, warlock_t* p, int id )
      : summon_pet_t( n, p, id )
    { ignore_false_positive = true; }

    summon_main_pet_t( util::string_view n, warlock_t* p )
      : summon_pet_t( n, p )
    { ignore_false_positive = true; }

    void schedule_execute( action_state_t* s = nullptr ) override
    {
      summon_pet_t::schedule_execute( s );

      if ( p()->warlock_pet_list.active )
      {
        p()->warlock_pet_list.active->dismiss();
        p()->warlock_pet_list.active = nullptr;
      }
    }

    virtual bool ready() override
    {
      if ( p()->warlock_pet_list.active == pet )
        return false;

      return summon_pet_t::ready();
    }

    virtual void execute() override
    {
      summon_pet_t::execute();

      p()->warlock_pet_list.active = pet;

      if ( p()->buffs.grimoire_of_sacrifice->check() )
        p()->buffs.grimoire_of_sacrifice->expire();
    }
  };

  struct drain_life_t : public warlock_spell_t
  {

    // Note: Soul Rot (Affliction talent) turns Drain Life into a multi-target channeled spell. Nothing else in simc behaves this way and
    // we currently do not have core support for it. Applying this dot to the secondary targets should cover most of the behavior, although
    // it will be unable to handle the case where primary channel target dies (in-game, this appears to force-swap primary target to another
    // target currently affected by Drain Life if possible).
    struct drain_life_dot_t : public warlock_spell_t
    {
      drain_life_dot_t( warlock_t* p )
        : warlock_spell_t( "Drain Life (AoE)", p, p->warlock_base.drain_life )
      { dual = background = true; }

      double cost_per_tick( resource_e ) const override
      { return 0.0; }
    };
    
    drain_life_dot_t* aoe_dot;

    drain_life_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Drain Life", p, p->warlock_base.drain_life, options_str )
    {
      aoe_dot = new drain_life_dot_t( p );
      add_child( aoe_dot );

      channeled = true;
    }

    void execute() override
    {
      warlock_spell_t::execute();


      p()->buffs.soulburn->expire();
    }

    double cost_per_tick( resource_e r ) const override
    {
      return warlock_spell_t::cost_per_tick( r );
    }

    void last_tick( dot_t* d ) override
    {
      bool early_cancel = d->remains() > 0_ms;

      warlock_spell_t::last_tick( d );

      // If this is the end of the channel, the AoE DoTs will expire correctly
      // Otherwise, we need to cancel them on the spot
    }
  };

  struct corruption_t : public warlock_spell_t
  {
    struct corruption_dot_t : public warlock_spell_t
    {
      corruption_dot_t( warlock_t* p )
        : warlock_spell_t( "Corruption", p, p->warlock_base.corruption->effectN( 1 ).trigger() )
      {
        tick_zero = false;
        background = dual = true;


      }

      void tick( dot_t* d ) override
      {
        warlock_spell_t::tick( d );

        if ( result_is_hit( d->state->result ) && p()->talents.nightfall.ok() )
          helpers::nightfall_updater( p(), d );
      }
    };

    corruption_dot_t* periodic;

    corruption_t( warlock_t* p, util::string_view options_str, bool seed_action )
      : warlock_spell_t( "Corruption (Direct)", p, p->warlock_base.corruption, options_str )
    {
      periodic = new corruption_dot_t( p );
      impact_action = periodic;
      add_child( periodic );

      spell_power_mod.direct = 0; // By default, Corruption does not deal instant damage

    }

    dot_t* get_dot( player_t* t ) override
    { return periodic->get_dot( t ); }
  };

  struct shadow_bolt_t : public warlock_spell_t
  {
    shadow_bolt_volley_t* volley;

    shadow_bolt_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Shadow Bolt", p, p->warlock_base.shadow_bolt, options_str )
    {
      affected_by.sacrificed_souls = true;
      triggers.shadow_invocation = true;
      triggers.jackpot_affliction = true;
      triggers.jackpot_demonology = true;

      // TODO: Fix this to Demonic power...
      if ( demonology() )
      {
        // energize_type = action_energize::ON_CAST;
        // energize_resource = RESOURCE_SOUL_SHARD;
        // energize_amount = 1.0;
      }

    }

    bool ready() override
    {
      return warlock_spell_t::ready();
    }

    double execute_time_pct_multiplier() const override
    {
      double m = warlock_spell_t::execute_time_pct_multiplier();

      return m;
    }

    void execute() override
    {
      warlock_spell_t::execute();

    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( result_is_hit( s->result ) )
      {
        // TODO: Validate for demo?
      }
    }

    double action_multiplier() const override
    {
      double m = warlock_spell_t::action_multiplier();

      return m;
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = warlock_spell_t::composite_target_multiplier( t );
      return m;
    }
  };

  struct grimoire_of_sacrifice_t : public warlock_spell_t
  {
    grimoire_of_sacrifice_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Grimoire of Sacrifice", p, p->talents.grimoire_of_sacrifice, options_str )
    {
      harmful = false;
      ignore_false_positive = true;
      target = player;
    }

    bool ready() override
    {
      if ( !p()->warlock_pet_list.active )
        return false;

      return warlock_spell_t::ready();
    }

    void execute() override
    {
      warlock_spell_t::execute();

      if ( p()->warlock_pet_list.active )
      {
        p()->warlock_pet_list.active->dismiss();
        p()->warlock_pet_list.active = nullptr;
        p()->buffs.grimoire_of_sacrifice->trigger();
      }
    }
  };

  struct grimoire_of_sacrifice_damage_t : public warlock_spell_t
  {
    grimoire_of_sacrifice_damage_t( warlock_t* p )
      : warlock_spell_t( "Grimoire of Sacrifice (Proc)", p, p->talents.grimoire_of_sacrifice_proc )
    {
      background = true;
      proc = true;

      triggers.decimation = false;

    }
  };

  struct soulburn_t : public warlock_spell_t
  {
    soulburn_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Soulburn", p, p->talents.soulburn, options_str )
    {
      harmful = false;
      may_crit = false;
    }

    bool ready() override
    {
      if ( p()->buffs.soulburn->check() )
        return false;

      return warlock_spell_t::ready();
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->buffs.soulburn->trigger();
    }
  };

  // Catchall action to trigger pet interrupt abilities via main APL.
  struct interrupt_t : public spell_t
  {
    interrupt_t( util::string_view n, warlock_t* p, util::string_view options_str ) :
      spell_t( n, p )
    {
      parse_options( options_str );
      callbacks = true;
      dual = usable_while_casting = true;
      may_miss = may_block = may_crit = false;
      ignore_false_positive = is_interrupt = true;
      trigger_gcd = 0_ms;
    }

    void execute() override
    {
      auto* w = debug_cast<warlock_t*>( player );

      auto pet = w->warlock_pet_list.active;

      switch( pet->pet_type )
      {
        case PET_FELGUARD:
        case PET_FELHUNTER:
          pet->special_action->execute_on_target( target );
          break;
        default:
          break;
      }

      spell_t::execute();
    }

    bool ready() override
    {
      auto* w = debug_cast<warlock_t*>( player );

      if ( !w->warlock_pet_list.active || w->warlock_pet_list.active->is_sleeping() )
        return false;

      auto pet = w->warlock_pet_list.active;

      switch( pet->pet_type )
      {
        case PET_FELGUARD:
        case PET_FELHUNTER:
          if ( !pet->special_action || !pet->special_action->cooldown->up() || !pet->special_action->ready() )
            return false;

          return spell_t::ready();
        default:
          return false;
      }
    }

    bool target_ready( player_t* candidate_target ) override
    {
      if ( !candidate_target->debuffs.casting || !candidate_target->debuffs.casting->check() )
        return false;

      return spell_t::target_ready( candidate_target );
    }
  };


  // Affliction Actions Begin

  // TODO: Canibalize this for MT
  // struct malefic_rapture_t : public warlock_spell_t
  // {
  //   struct malefic_touch_t : public warlock_spell_t
  //   {
  //     malefic_touch_t( warlock_t* p )
  //       : warlock_spell_t( "Malefic Touch", p, p->talents.malefic_touch_proc )
  //     {
  //       background = dual = true;

  //       base_dd_multiplier *= 1.0 + p->talents.kindled_malice->effectN( 1 ).percent();
  //       base_dd_multiplier *= 1.0 + p->talents.improved_malefic_rapture->effectN( 1 ).percent();

  //       if ( active_2pc( TWW1 ) )
  //         base_dd_multiplier *= 1.0 + p->tier.hexflame_aff_2pc->effectN( 1 ).percent();
  //     }

  //     double composite_crit_chance_multiplier() const override
  //     {
  //       double m = warlock_spell_t::composite_crit_chance_multiplier();

  //       if ( active_2pc( TWW1 ) )
  //         m *= 1.0 + p()->tier.hexflame_aff_2pc->effectN( 2 ).percent();

  //       return m;
  //     }
  //   };

  //   struct malefic_rapture_damage_t : public warlock_spell_t
  //   {
  //     int target_count;
  //     malefic_touch_t* touch;

  //     malefic_rapture_damage_t( warlock_t* p )
  //       : warlock_spell_t ( "Malefic Rapture (hit)", p, p->warlock_base.malefic_rapture_dmg ),
  //       target_count( 0 )
  //     {
  //       background = dual = true;
  //       callbacks = false; // Individual hits have been observed to not proc trinkets like Psyche Shredder

  //       base_dd_multiplier *= 1.0 + p->talents.kindled_malice->effectN( 1 ).percent();
  //       base_dd_multiplier *= 1.0 + p->talents.improved_malefic_rapture->effectN( 1 ).percent();

  //       if ( active_2pc( TWW1 ) )
  //         base_dd_multiplier *= 1.0 + p->tier.hexflame_aff_2pc->effectN( 1 ).percent();

  //       affected_by.deaths_embrace = p->talents.deaths_embrace.ok();

  //       if ( p->talents.malefic_touch.ok() )
  //       {
  //         touch = new malefic_touch_t( p );
  //         add_child( touch );
  //       }
  //     }

  //     double composite_da_multiplier( const action_state_t* s ) const override
  //     {
  //       double m = warlock_spell_t::composite_da_multiplier( s );

  //       m *= td( s->target )->count_affliction_dots( true );

  //       if ( p()->talents.focused_malignancy.ok() && ( td( s->target )->dots_unstable_affliction->is_ticking() || td( s->target )->dots_jackpot_ua->is_ticking() ) )
  //         m *= 1.0 + p()->talents.focused_malignancy->effectN( 1 ).percent();

  //       if ( p()->talents.cull_the_weak.ok() )
  //         m *= 1.0 + ( std::min( target_count, as<int>( p()->talents.cull_the_weak->effectN( 2 ).base_value() ) ) * p()->talents.cull_the_weak->effectN( 1 ).percent() );

  //       if ( p()->talents.malign_omen.ok() )
  //         m *= 1.0 + p()->buffs.malign_omen->check_value();

  //       if ( soul_harvester() && p()->buffs.succulent_soul->check() )
  //         m *= 1.0 + p()->hero.succulent_soul->effectN( 2 ).percent();

  //       return m;
  //     }

  //     void execute() override
  //     {
  //       int d = td( target )->count_affliction_dots( true ) - 1;
  //       assert( d < as<int>( p()->procs.malefic_rapture.size() ) && "The procs.malefic_rapture array needs to be expanded." );

  //       if ( d >= 0 && d < as<int>( p()->procs.malefic_rapture.size() ) )
  //         p()->procs.malefic_rapture[ d ]->occur();

  //       warlock_spell_t::execute();
  //     }

  //     void impact( action_state_t* s ) override
  //     {
  //       warlock_spell_t::impact( s );

  //       if ( p()->buffs.malign_omen->check() )
  //       {
  //         warlock_td_t* tdata = td( s->target );
  //         timespan_t extension = timespan_t::from_seconds( p()->talents.malign_omen_buff->effectN( 2 ).base_value() );

  //         tdata->dots_agony->adjust_duration( extension );
  //         tdata->dots_corruption->adjust_duration( extension );
  //         tdata->dots_phantom_singularity->adjust_duration( extension );
  //         tdata->dots_vile_taint->adjust_duration( extension );
  //         tdata->dots_unstable_affliction->adjust_duration( extension );
  //         tdata->dots_jackpot_ua->adjust_duration( extension );
  //         tdata->dots_soul_rot->adjust_duration( extension );
  //         tdata->debuffs_haunt->extend_duration( p(), extension );
  //         tdata->dots_wither->adjust_duration( extension );
  //       }

  //       if ( p()->talents.malefic_touch.ok() )
  //         touch->execute_on_target( s->target );

  //       if ( soul_harvester() && p()->buffs.succulent_soul->check() )
  //       {
  //         bool fervor = td( s->target )->dots_unstable_affliction->is_ticking();

  //         if ( !p()->bugs )
  //           fervor |= td( s->target )->dots_jackpot_ua->is_ticking();

  //         debug_cast<demonic_soul_t*>( p()->proc_actions.demonic_soul )->demoniacs_fervor = fervor;
  //         p()->proc_actions.demonic_soul->execute_on_target( s->target );
  //       }
  //     }

  //     double composite_crit_chance_multiplier() const override
  //     {
  //       double m = warlock_spell_t::composite_crit_chance_multiplier();

  //       if ( active_2pc( TWW1 ) )
  //         m *= 1.0 + p()->tier.hexflame_aff_2pc->effectN( 2 ).percent();

  //       return m;
  //     }
  //   };

  //   malefic_rapture_t( warlock_t* p, util::string_view options_str )
  //     : warlock_spell_t( "Malefic Rapture", p, p->warlock_base.malefic_rapture, options_str )
  //   {
  //     aoe = -1;

  //     triggers.jackpot_affliction = true;

  //     impact_action = new malefic_rapture_damage_t( p );
  //     add_child( impact_action );
  //   }

  //   double cost_pct_multiplier() const override
  //   {
  //     double c = warlock_spell_t::cost_pct_multiplier();

  //     if ( p()->buffs.tormented_crescendo->check() )
  //       c *= 1.0 + p()->talents.tormented_crescendo_buff->effectN( 3 ).percent();

  //     return c;
  //   }

  //   double execute_time_pct_multiplier() const override
  //   {
  //     double m = warlock_spell_t::execute_time_pct_multiplier();

  //     if ( p()->buffs.tormented_crescendo->check() )
  //       m *= 1.0 + p()->talents.tormented_crescendo_buff->effectN( 2 ).percent();

  //     m *= 1.0 + p()->talents.improved_malefic_rapture->effectN( 2 ).percent();

  //     return m;
  //   }

  //   bool ready() override
  //   {
  //     if ( !warlock_spell_t::ready() )
  //       return false;

  //     target_cache.is_valid = false;
  //     return target_list().size() > 0;
  //   }

  //   void execute() override
  //   {
  //     warlock_spell_t::execute();

  //     if ( p()->talents.malign_omen.ok() && p()->buffs.malign_omen->check() )
  //       p()->buffs.soul_rot->extend_duration( p(), timespan_t::from_seconds( p()->talents.malign_omen_buff->effectN( 2 ).base_value() ) );

  //     if ( active_4pc( TWW1 ) )
  //     {
  //       bool success = p()->buffs.umbral_lattice->trigger();

  //       if ( success )
  //         p()->procs.umbral_lattice->occur();
  //     }

  //     p()->buffs.tormented_crescendo->decrement();
  //     p()->buffs.malign_omen->decrement();
  //   }

  //   void impact( action_state_t* s ) override
  //   {
  //     warlock_spell_t::impact( s );

  //     debug_cast<malefic_rapture_damage_t*>( impact_action )->target_count = as<int>( s->n_targets );

  //     if ( soul_harvester() && p()->buffs.succulent_soul->check() )
  //     {
  //       bool primary = ( s->chain_target == 0 );

  //       if ( primary )
  //         make_event( *sim, 1_ms, [ this ] { p()->buffs.succulent_soul->decrement(); } );
  //     }
  //   }

  //   size_t available_targets( std::vector<player_t*>& tl ) const override
  //   {
  //     warlock_spell_t::available_targets( tl );

  //     range::erase_remove( tl, [ this ]( player_t* t ){ return td( t )->count_affliction_dots( true ) == 0; } );

  //     return tl.size();
  //   }
  // };
  struct unstable_affliction_t : public warlock_spell_t
  {
    perpetual_unstability_t* perpetual_unstability;

    unstable_affliction_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Unstable Affliction", p, p->talents.unstable_affliction, options_str )
    {
      base_dd_multiplier *= 1.0 + p->talents.xavius_gambit->effectN( 2 ).percent();
      base_td_multiplier *= 1.0 + p->talents.xavius_gambit->effectN( 1 ).percent();

      dot_duration += p->talents.unstable_affliction_3->effectN( 1 ).time_value();

      triggers.ravenous_afflictions = p->talents.ravenous_afflictions.ok();

      affected_by.deaths_embrace = p->talents.deaths_embrace.ok();

      if ( p->talents.perpetual_unstability.ok() )
      {
        perpetual_unstability = new perpetual_unstability_t( p );
        add_child( perpetual_unstability );
      }
    }

    double execute_time_pct_multiplier() const override
    {
      double m = warlock_spell_t::execute_time_pct_multiplier();

      m *= 1.0 + p()->talents.perpetual_unstability->effectN( 2 ).percent();

      return m;
    }

    void execute() override
    {
      if ( p()->ua_target && p()->ua_target != target )
        td( p()->ua_target )->dots_unstable_affliction->cancel();

      if ( active_4pc( TWW2 ) && td( target )->dots_jackpot_ua->is_ticking() )
        td( target )->dots_jackpot_ua->cancel();

      p()->ua_target = target;

      warlock_spell_t::execute();
    }

    void impact( action_state_t* s ) override
    {
      bool ticking = td( s->target )->dots_unstable_affliction->is_ticking();
      timespan_t remains = td( s->target )->dots_unstable_affliction->remains();

      warlock_spell_t::impact( s );

      if ( p()->talents.perpetual_unstability.ok() && ticking && remains < timespan_t::from_seconds( p()->talents.perpetual_unstability->effectN( 1 ).base_value() ) )
        perpetual_unstability->execute_on_target( s->target );
    }

    void last_tick( dot_t* d ) override
    {
      warlock_spell_t::last_tick( d );

      p()->ua_target = nullptr;
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_ta_multiplier( s );

      if ( active_4pc( TWW2 ) && p()->buffs.jackpot_affliction->check() )
        m *= 1.0 + p()->tier.spliced_aff_4pc->effectN( 1 ).percent();

      return m;
    }
  };

  struct agony_t : public warlock_spell_t
  {
    agony_t( warlock_t* p, util::string_view options_str ) 
      : warlock_spell_t( "Agony", p, p->warlock_base.agony, options_str )
    {
      may_crit = false;

      dot_max_stack = as<int>( data().max_stacks() );

    }

    void last_tick ( dot_t* d ) override
    {
      if ( p()->get_active_dots( d ) == 1 )
        p()->agony_accumulator = rng().range( 0.0, 0.99 );

      warlock_spell_t::last_tick( d );
    }

    void execute() override
    {
      warlock_spell_t::execute();

    }

    void impact( action_state_t* s ) override
    {

      warlock_spell_t::impact( s );
    }

    void tick( dot_t* d ) override
    {
      // Blizzard has not publicly released the formula for Agony's chance to generate a Soul Shard.
      // This set of code is based on results from 500+ Soul Shard sample sizes, and matches in-game
      // results to within 0.1% of accuracy in all tests conducted on all targets numbers up to 8.
      // Accurate as of 08-24-2018. TOCHECK regularly. If any changes are made to this section of
      // code, please also update the Time_to_Shard expression in sc_warlock.cpp.
      double increment_max = p()->rng_settings.agony.setting_value;

      double active_agonies = p()->get_active_dots( d );
      increment_max *= std::pow( active_agonies, -2.0 / 3.0 );

      // 2023-09-01: Recent test noted that Creeping Death is once again renormalizing shard generation to be neutral with/without the talent.
      p()->agony_accumulator += rng().range( 0.0, increment_max );


      warlock_spell_t::tick( d );

      td( d->state->target )->dots_agony->increment( 1 );
    }
  };

  struct seed_of_corruption_t : public warlock_spell_t
  {
    struct seed_of_corruption_aoe_t : public warlock_spell_t
    {
      action_t* applied_dot;

      seed_of_corruption_aoe_t( warlock_t* p )
        : warlock_spell_t( "Seed of Corruption (AoE)", p, p->warlock_base.seed_of_corruption_aoe, options_str )
      {
        aoe = -1;
        background = dual = true;
        applied_dot = new corruption_t( p, "", true );

        applied_dot->background = true;
        applied_dot->dual = true;
        applied_dot->base_costs[ RESOURCE_MANA ] = 0;
        applied_dot->base_dd_multiplier = 0.0;

      }

      void impact( action_state_t* s ) override
      {
        warlock_spell_t::impact( s );

        if ( result_is_hit( s->result ) )
        {
          auto tdata = td( s->target );

          if ( tdata->dots_seed_of_corruption->is_ticking() && tdata->soc_threshold > 0 )
          {
            tdata->soc_threshold = 0;
            tdata->dots_seed_of_corruption->cancel();
          }
          
          applied_dot->execute_on_target( s->target );
        }
      }
    };

    seed_of_corruption_aoe_t* explosion;

    seed_of_corruption_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Seed of Corruption", p, p->talents.seed_of_corruption, options_str ),
      explosion( new seed_of_corruption_aoe_t( p ) )
    {
      may_crit = false;
      tick_zero = false;
      base_tick_time = dot_duration;
      hasted_ticks = false;

      add_child( explosion );
    }

    void init() override
    {
      warlock_spell_t::init();
      snapshot_flags |= STATE_SP;
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      warlock_spell_t::available_targets( tl );

      // Targeting behavior appears to be as follows:
      // 1. If any targets have no current seed (in flight or ticking), they are valid
      // 2. If no targets are valid according to the above, all targets are instead valid (will refresh DoT on existing target(s) instead)
      bool valid_target = false;
      for ( auto t : tl )
      {
        if ( !( td( t )->dots_seed_of_corruption->is_ticking() || has_travel_events_for( t ) ) )
        {
          valid_target = true;
          break;
        }
      }

      if ( valid_target )
      {
        // TODO: ranges is bugging out here for some reason?
        // range::erase_remove( tl, [ this ]( player_t* t ) {
        //   return ( td( t )->dots_seed_of_corruption->is_ticking() || has_travel_events_for( t ) );
        // } );
      }

      return tl.size();
    }

    void impact( action_state_t* s ) override
    {
      // TODO: setup the damage threashold stuff here.
      // if ( result_is_hit( s->result ) )
      //   td( s->target )->soc_threshold = s->composite_spell_power() * p()->talents.seed_of_corruption->effectN( 1 ).percent();

      warlock_spell_t::impact( s );
    }

    // If Seed of Corruption is refreshed on a target, it will extend the duration
    // but still explode at the original time, wiping the "DoT". tick() should be used instead
    // of last_tick() to model this appropriately.
    void tick( dot_t* d ) override
    {
      warlock_spell_t::tick( d );

      if ( d->remains() > 0_ms )
        d->cancel();
    }

    void last_tick( dot_t* d ) override
    {
      explosion->set_target( d->target );
      explosion->schedule_execute();

      warlock_spell_t::last_tick( d );
    }
  };

  struct drain_soul_t : public warlock_spell_t
  {
    struct drain_soul_state_t : public action_state_t
    {
      double tick_time_multiplier;
      double td_multiplier;

      drain_soul_state_t( action_t* action, player_t* target )
        : action_state_t( action, target ),
        tick_time_multiplier( 1.0 ),
        td_multiplier( 1.0 )
      { }

      void initialize() override
      {
        action_state_t::initialize();
        tick_time_multiplier = 1.0;
        td_multiplier = 1.0;
      }

      std::ostringstream& debug_str( std::ostringstream& s ) override
      {
        action_state_t::debug_str( s ) << " tick_time_multiplier=" << tick_time_multiplier;
        action_state_t::debug_str( s ) << " td_multiplier=" << td_multiplier;
        return s;
      }

      void copy_state( const action_state_t* s ) override
      {
        action_state_t::copy_state( s );
        tick_time_multiplier = debug_cast<const drain_soul_state_t*>( s )->tick_time_multiplier;
        td_multiplier = debug_cast<const drain_soul_state_t*>( s )->td_multiplier;
      }
    };

    // TODO: correct this.
    drain_soul_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Drain Soul", p, options_str )
    {
      channeled = true;

    }

    action_state_t* new_state() override
    { return new drain_soul_state_t( this, target ); }

    void snapshot_state( action_state_t* s, result_amount_type rt ) override
    {
      warlock_spell_t::snapshot_state( s, rt );
    }

    double tick_time_pct_multiplier( const action_state_t* s ) const override
    {
      auto mul = warlock_spell_t::tick_time_pct_multiplier( s );

      mul *= debug_cast<const drain_soul_state_t*>( s )->tick_time_multiplier;

      return mul;
    }

    double dot_duration_pct_multiplier( const action_state_t* s ) const override
    {
      auto mul = warlock_spell_t::dot_duration_pct_multiplier( s );

      return mul;
    }

    void execute() override
    {
      warlock_spell_t::execute();

    }

    void tick( dot_t* d ) override
    {
      warlock_spell_t::tick( d );

      if ( result_is_hit( d->state->result ) )
      {
        // TODO: tick all dots
        return;
      }
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = warlock_spell_t::composite_target_multiplier( t );


      return m;
    }

    double composite_ta_multiplier( const action_state_t* s ) const override
    {
      double m = warlock_spell_t::composite_ta_multiplier( s );

      m *= debug_cast<const drain_soul_state_t*>( s )->td_multiplier;

      return m;
    }
  };

  struct haunt_t : public warlock_spell_t
  {
    // TODO: fix
    haunt_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Haunt", p, nullptr, options_str )
    {
      triggers.jackpot_affliction = true;
    }

    double execute_time_pct_multiplier() const override
    {
      double m = warlock_spell_t::execute_time_pct_multiplier();


      return m;
    }

    void execute() override
    {
      warlock_spell_t::execute();

    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( result_is_hit( s->result ) )
      {
        td( s->target )->debuffs_haunt->trigger();

      }
    }
  };

  // Affliction Actions End
  // Demonology Actions Begin

  struct hand_of_guldan_t : public warlock_spell_t
  {
    struct hog_impact_t : public warlock_spell_t
    {
      int shards_used;
      timespan_t meteor_time;
      umbral_blaze_dot_t* blaze;

      hog_impact_t( warlock_t* p )
        : warlock_spell_t( "Hand of Gul'dan (Impact)", p, p->warlock_base.hog_impact ),
        shards_used( 0 ),
        meteor_time( 400_ms )
      {
        aoe = -1;
        dual = true;

        affected_by.touch_of_rancora = p->hero.touch_of_rancora.ok();
        
        triggers.shadow_invocation = true;
        triggers.demonic_art = p->hero.diabolic_ritual.ok();

        if ( p->talents.umbral_blaze.ok() )
        {
          blaze = new umbral_blaze_dot_t( p );
          add_child( blaze );
        }
      }

      timespan_t travel_time() const override
      { return meteor_time; }

      double action_multiplier() const override
      {
        double m = warlock_spell_t::action_multiplier();

        double gloom = 0.0;

        if ( p()->hero.gloom_of_nathreza.ok() )
           gloom = shards_used * p()->hero.gloom_of_nathreza->effectN( 1 ).percent();

        m *= shards_used * ( 1.0 + gloom );

        if ( soul_harvester() && p()->buffs.succulent_soul->check() )
          m *= 1.0 + p()->hero.succulent_soul->effectN( 3 ).percent();

        // NOTE: Touch of Rancora is a +100% ADDITION to the MULTIPLIER, we currently believe this must be done at the end of calculation
        if ( diabolist() && affected_by.touch_of_rancora )
        {
          if ( p()->buffs.art_overlord->check() )
            m += p()->hero.touch_of_rancora->effectN( 1 ).percent();

          if ( p()->buffs.art_mother->check() )
            m += p()->hero.touch_of_rancora->effectN( 1 ).percent();

          if ( p()->buffs.art_pit_lord->check() )
            m += p()->hero.touch_of_rancora->effectN( 1 ).percent();
        }

        return m;
      }

      void impact( action_state_t* s ) override
      {
        warlock_spell_t::impact( s );

        // Only trigger Wild Imps once for the original target impact.
        // Still keep it in impact instead of execute because of travel delay.
        if ( result_is_hit( s->result ) && s->target == target )
        {
          // Wild Imp spawns appear to have been sped up in Shadowlands. Last tested 2021-04-16.
          // Current behavior: HoG will spawn a meteor on cast finish. Travel time in spell data is 0.7 seconds.
          // However, damage event occurs before spell effect lands, happening 0.4 seconds after cast.
          // Imps then spawn roughly every 0.18 seconds seconds after the damage event.
          for ( int i = 1; i <= shards_used; i++ )
          {
            auto ev = make_event<imp_delay_event_t>( *sim, p(), rng().gauss( 180.0 * i, 25.0 ), 180.0 * i );
            p()->wild_imp_spawns.push_back( ev );
          }

          if ( p()->talents.umbral_blaze.ok() && rng().roll( p()->talents.umbral_blaze->effectN( 1 ).percent() ) )
          {
            blaze->execute_on_target( s->target );
            p()->procs.umbral_blaze->occur();
          }
        }

        // We need Demonic Soul to proc on every target, but buff is decremented on impact. Fudge this by 1ms to ensure all targets are hit.
        if ( soul_harvester() && p()->buffs.succulent_soul->check() )
        {
          bool primary = ( s->chain_target == 0 );

          if ( primary )
            make_event( *sim, 1_ms, [ this ] { p()->buffs.succulent_soul->decrement(); } );

          debug_cast<demonic_soul_t*>(p()->proc_actions.demonic_soul)->demoniacs_fervor = primary;
          p()->proc_actions.demonic_soul->execute_on_target( s->target );
        }
      }
    };

    hog_impact_t* impact_spell;

    hand_of_guldan_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Hand of Gul'dan", p, p->warlock_base.hand_of_guldan, options_str ),
      impact_spell( new hog_impact_t( p ) )
    {
      add_child( impact_spell );
    }

    // TODO: validate
    timespan_t travel_time() const override
    { return 0_ms; }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      impact_spell->execute_on_target( s->target );
    }
  };

  struct grimoire_felguard_t : public warlock_spell_t
  {
    grimoire_felguard_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Grimoire: Felguard", p, p->talents.grimoire_felguard, options_str )
    {
      harmful = may_crit = false;
    }

    void execute() override
    {
      warlock_spell_t::execute();
    }
  };

  struct doom_t : public warlock_spell_t
  {
    doom_t( warlock_t* p )
      : warlock_spell_t( "Doom", p, p->talents.doom_dmg )
    {
      background = dual = true;
      aoe = -1;
    }

    void execute() override
    {
      warlock_spell_t::execute();

    }
  };

  // Demonology Actions End
  // Destruction Actions Begin

  // TODO: Need to figure out embers for this mess....
  struct incinerate_t : public warlock_spell_t
  {
    struct incinerate_fnb_t : public warlock_spell_t
    {
      incinerate_fnb_t( warlock_t* p )
        : warlock_spell_t( "Incinerate (Fire and Brimstone)", p, p->warlock_base.incinerate )
      {
        aoe = -1;
        background = dual = true;
      }

      void init() override
      {
        warlock_spell_t::init();

        p()->havoc_spells.push_back( this ); // Needed for proper target list invalidation
      }

      double cost() const override
      { return 0.0; }

      size_t available_targets( std::vector<player_t*>& tl ) const override
      {
        warlock_spell_t::available_targets( tl );

        auto it = range::find( tl, target );
        if ( it != tl.end() )
          tl.erase( it );

        it = range::find( tl, p()->havoc_target );
        if ( it != tl.end() )
          tl.erase( it );

        return tl.size();
      }
    };

    double energize_mult;
    incinerate_fnb_t* fnb_action;

    incinerate_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Incinerate", p, p->warlock_base.incinerate, options_str ),
      fnb_action( new incinerate_fnb_t( p ) )
    {
      energize_type = action_energize::PER_HIT;
      energize_resource = RESOURCE_SOUL_SHARD;
      // energize_amount = ( p->warlock_base.incinerate_energize->effectN( 1 ).base_value() ) / 10.0;
      
      affected_by.havoc = true;

      add_child( fnb_action );
    }


    void execute() override
    {
      warlock_spell_t::execute();
    
      p()->buffs.backdraft->decrement();
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( s->result == RESULT_CRIT )
        p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1 * energize_mult, p()->gains.incinerate_crits );
    }
  };

  struct immolate_t : public warlock_spell_t
  {
    struct immolate_dot_t : public warlock_spell_t
    {
      immolate_dot_t( warlock_t* p )
        : warlock_spell_t( "Immolate", p, p->warlock_base.immolate_dot )
      {
        background = dual = true;

        affected_by.chaotic_energies = true;
      }

      void tick( dot_t* d ) override
      {
        warlock_spell_t::tick( d );

        if ( d->state->result == RESULT_CRIT && rng().roll( p()->warlock_base.immolate_old->effectN( 2 ).percent() ) )
          p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.immolate_crits );

        p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.immolate );

      }
    };

    immolate_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Immolate (direct)", p, p->warlock_base.immolate->ok() && !p->hero.wither.ok() ? p->warlock_base.immolate_old : spell_data_t::not_found(), options_str )
    {
      affected_by.chaotic_energies = true;
      affected_by.havoc = true;

      triggers.jackpot_destruction = true;

      impact_action = new immolate_dot_t( p );
      add_child( impact_action );
    }

    immolate_t( warlock_t* p, bool havoc, util::string_view options_str ) : immolate_t( p, options_str )
    { affected_by.havoc = havoc; }

    dot_t* get_dot( player_t* t ) override
    { return impact_action->get_dot( t ); }
  };

  struct chaos_bolt_t : public warlock_spell_t
  {
    chaos_bolt_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Chaos Bolt", p, p->warlock_base.chaos_bolt, options_str )
    {
    }

    void execute() override
    {
      warlock_spell_t::execute();

      p()->buffs.backdraft->decrement(3);
    }

  };

  struct conflagrate_t : public warlock_spell_t
  {
    conflagrate_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Conflagrate", p, p->talents.conflagrate, options_str )
    {
      affected_by.chaotic_energies = true;
      affected_by.havoc = true;

      triggers.jackpot_destruction = true;

      energize_type = action_energize::PER_HIT;
      energize_resource = RESOURCE_SOUL_SHARD;
      // energize_amount = ( p->talents.conflagrate_2->effectN( 1 ).base_value() ) / 10.0;

      cooldown->hasted = true;
      // cooldown->charges += as<int>( p->talents.improved_conflagrate->effectN( 1 ).base_value() );
      // cooldown->duration += p->talents.explosive_potential->effectN( 1 ).time_value();
    }


    void execute() override
    {
      warlock_spell_t::execute();

      p()->buffs.backdraft->trigger(3);
    }

  };

  struct rain_of_fire_t : public warlock_spell_t
  {
    struct rain_of_fire_tick_t : public warlock_spell_t
    {
      rain_of_fire_tick_t( warlock_t* p )
        : warlock_spell_t( "Rain of Fire (tick)", p, p->talents.rain_of_fire_tick )
      {
        background = dual = true;
        aoe = -1;
        // radius = p->talents.rain_of_fire->effectN( 1 ).radius();

        affected_by.chaotic_energies = true;
        triggers.decimation = false;
      }
    };

    rain_of_fire_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Rain of Fire", p, p->talents.rain_of_fire, options_str )
    {
      may_miss = may_crit = false;
      base_tick_time = 1_s;
      dot_duration = 0_s;
      aoe = -1; // Needed to apply Pyrogenics

      // base_costs[ RESOURCE_SOUL_SHARD ] += p->talents.inferno->effectN( 1 ).base_value() / 10.0;

      if ( !p->proc_actions.rain_of_fire_tick )
      {
        p->proc_actions.rain_of_fire_tick = new rain_of_fire_tick_t( p );
        p->proc_actions.rain_of_fire_tick->stats = stats;
      }
    }

    void execute() override
    {
      warlock_spell_t::execute();

    }
  };

  struct havoc_t : public warlock_spell_t
  {
    havoc_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Havoc", p, p->talents.havoc, options_str )
    { may_crit = false; }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      td( s->target )->debuffs_havoc->trigger();
    }
  };

  struct shadowburn_t : public warlock_spell_t
  {
    shadowburn_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Shadowburn", p, p->talents.shadowburn, options_str )
    {
      cooldown->hasted = true;
      
      affected_by.havoc = true;
    }

    void impact( action_state_t* s ) override
    {
      warlock_spell_t::impact( s );

      if ( result_is_hit( s->result ) )
      {
        td( s->target )->debuffs_shadowburn->trigger();
      }
    }

  };

  struct summon_infernal_t : public warlock_spell_t
  {
    summon_infernal_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Summon Infernal", p, p->talents.summon_infernal, options_str )
    {
      may_crit = false;
      resource_current = RESOURCE_SOUL_SHARD; // For Cruelty of Kerxan proccing

      add_child( impact_action );
    }

    void execute() override
    {
      warlock_spell_t::execute();

      // if ( p()->talents.crashing_chaos.ok() )
      //   p()->buffs.crashing_chaos->trigger();

      // if ( p()->talents.rain_of_chaos.ok() )
      //   p()->buffs.rain_of_chaos->trigger();

      // if ( p()->hero.cruelty_of_kerxan.ok() )
      // {
      //   timespan_t reduction = -p()->hero.cruelty_of_kerxan->effectN( 1 ).time_value();

      //   p()->buffs.ritual_overlord->extend_duration( p(), reduction );
      //   p()->buffs.ritual_mother->extend_duration( p(), reduction );
      //   p()->buffs.ritual_pit_lord->extend_duration( p(), reduction );
      // }

      // if ( active_2pc( TWW2 ) )
      // {
      //   p()->buffs.demonfire_flurry_trigger->trigger();
      //   p()->procs.jackpot_destruction->occur();

      //   if ( active_4pc( TWW2 ) )
      //     p()->buffs.jackpot_destruction->trigger();
      // }
    }
  };

  // TODO: this is the demo soulfire atm, this should auto crit & scale off of crit damage.
  struct soul_fire_t : public warlock_spell_t
  {
    action_t* applied_dot;

    soul_fire_t( warlock_t* p, util::string_view options_str )
      : warlock_spell_t( "Soul Fire", p, p->talents.soul_fire, options_str )
    {
      energize_type = action_energize::PER_HIT;
      energize_resource = RESOURCE_SOUL_SHARD;
      // energize_amount = ( p->talents.soul_fire_2->effectN( 1 ).base_value() ) / 10.0;

      affected_by.chaotic_energies = true;
      affected_by.havoc = true;

      triggers.jackpot_destruction = true;


      applied_dot->background = true;
      applied_dot->dual = true;
      applied_dot->base_costs[ RESOURCE_MANA ] = 0;
      applied_dot->base_dd_multiplier = 0.0;
    }

    double execute_time_pct_multiplier() const override
    {
      double m = warlock_spell_t::execute_time_pct_multiplier();

      return m;
    }

    void execute() override
    {
      warlock_spell_t::execute();

      applied_dot->execute_on_target( target );

      p()->buffs.backdraft->decrement();
    }

    double composite_crit_chance() const override
    {
      double c = warlock_spell_t::composite_crit_chance();

      return c;
    }
  };

  // Destruction Actions End

  // Helper Functions Begin


  void helpers::nightfall_updater( warlock_t* p, dot_t* d )
  {
    // Blizzard did not publicly release how nightfall was changed.
    // We determined this is the probable functionality copied from Agony by first confirming the
    // DR formula was the same and then confirming that you can get procs on 1st tick.
    // The procs also have a regularity that suggest it does not use a proc chance or rppm.
    // Last checked 09-28-2020.
    double increment_max = p->rng_settings.nightfall.setting_value;

    double active_corruptions = p->get_active_dots( d );
    increment_max *= std::pow( active_corruptions, -2.0 / 3.0 );

    p->corruption_accumulator += p->rng().range( 0.0, increment_max );

    if ( p->corruption_accumulator >= 1 )
    {
      p->procs.nightfall->occur();
      p->buffs.nightfall->trigger();
      p->corruption_accumulator -= 1.0;
    }
  }

  // Event for spawning Wild Imps for Demonology
  imp_delay_event_t::imp_delay_event_t( warlock_t* p, double delay, double exp ) : player_event_t( *p, timespan_t::from_millis( delay ) )
  { diff = timespan_t::from_millis( exp - delay ); }

  const char* imp_delay_event_t::name() const
  { return "imp_delay"; }

  void imp_delay_event_t::execute()
  {
    warlock_t* p = static_cast<warlock_t*>( player() );

    p->warlock_pet_list.wild_imps.spawn();

    // Remove this event from the vector
    auto it = std::find( p->wild_imp_spawns.begin(), p->wild_imp_spawns.end(), this );
    if ( it != p->wild_imp_spawns.end() )
      p->wild_imp_spawns.erase( it );
  }

  // Used for APL expressions to estimate when imp is "supposed" to spawn
  timespan_t imp_delay_event_t::expected_time()
  { return std::max( 0_ms, this->remains() + diff ); }

  // Helper Functions End
  
  // Action Creation Begin

  action_t* warlock_t::create_action( util::string_view action_name, util::string_view options_str )
  {
    if ( specialization() == WARLOCK_AFFLICTION )
    {
      if ( action_t* aff_action = create_action_affliction( action_name, options_str ) )
        return aff_action;
    }

    if ( specialization() == WARLOCK_DEMONOLOGY )
    {
      if ( action_t* demo_action = create_action_demonology( action_name, options_str ) )
        return demo_action;
    }

    if ( specialization() == WARLOCK_DESTRUCTION )
    {
      if ( action_t* destro_action = create_action_destruction( action_name, options_str ) )
        return destro_action;
    }

    if ( action_t* generic_action = create_action_warlock( action_name, options_str ) )
      return generic_action;

    return player_t::create_action( action_name, options_str );
  }


  // TODO: correct where these actions are.
  action_t* warlock_t::create_action_warlock( util::string_view action_name, util::string_view options_str )
  {
    if ( ( action_name == "summon_pet" ) && default_pet.empty() )
    {
      sim->errorf( "Player %s used a generic pet summoning action without specifying a default_pet.\n", name() );
      return nullptr;
    }

    // Pets
    if ( action_name == "summon_felhunter" )
      return new summon_main_pet_t( "felhunter", this );
    if ( action_name == "summon_felguard" && specialization() == WARLOCK_DEMONOLOGY )
      return new summon_main_pet_t( "felguard", this );
    if ( action_name == "summon_sayaad" )
      return new summon_main_pet_t( "sayaad", this, 366222 );
    if ( action_name == "summon_succubus" )
      return new summon_main_pet_t( "succubus", this, 366222 );
    if ( action_name  == "summon_incubus" )
      return new summon_main_pet_t( "incubus", this, 366222 );
    if ( action_name == "summon_voidwalker" )
      return new summon_main_pet_t( "voidwalker", this );
    if ( action_name == "summon_imp" )
      return new summon_main_pet_t( "imp", this );
    if ( action_name == "summon_pet" )
    {
      if ( default_pet == "sayaad" || default_pet == "succubus" || default_pet == "incubus" )
        return new summon_main_pet_t( default_pet, this, 366222 );

      return new summon_main_pet_t( default_pet, this );
    }

    // Shared Spells
    if ( action_name == "drain_life" )
      return new drain_life_t( this, options_str );
    if ( action_name == "corruption" && specialization() != WARLOCK_DESTRUCTION )
      return new corruption_t( this, options_str, false );
    if ( action_name == "shadow_bolt" && specialization() != WARLOCK_DESTRUCTION )
      return new shadow_bolt_t( this, options_str );
    if ( action_name == "grimoire_of_sacrifice" && specialization() != WARLOCK_DEMONOLOGY )
      return new grimoire_of_sacrifice_t( this, options_str );
    if ( action_name == "interrupt" )
      return new interrupt_t( action_name, this, options_str );
    if ( action_name == "soulburn" )
      return new soulburn_t( this, options_str );

    return nullptr;
  }

  action_t* warlock_t::create_action_affliction( util::string_view action_name, util::string_view options_str )
  {
    if ( action_name == "agony" )
      return new agony_t( this, options_str );
    if ( action_name == "unstable_affliction" )
      return new unstable_affliction_t( this, options_str );
    if ( action_name == "drain_soul" )
      return new drain_soul_t( this, options_str );
    if ( action_name == "haunt" )
      return new haunt_t( this, options_str );
    if ( action_name == "seed_of_corruption" )
      return new seed_of_corruption_t( this, options_str );

    return nullptr;
  }

  action_t* warlock_t::create_action_demonology( util::string_view action_name, util::string_view options_str )
  {
    if ( action_name == "hand_of_guldan" )
      return new hand_of_guldan_t( this, options_str );
    if ( action_name == "grimoire_felguard" )
      return new grimoire_felguard_t( this, options_str );
    return nullptr;
  }

  action_t* warlock_t::create_action_destruction( util::string_view action_name, util::string_view options_str )
  {
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
    if ( action_name == "soul_fire" )
      return new soul_fire_t( this, options_str );
    if ( action_name == "shadowburn" )
      return new shadowburn_t( this, options_str );

    return nullptr;
  }

  void warlock_t::create_actions()
  {
    if ( specialization() == WARLOCK_AFFLICTION )
      create_affliction_proc_actions();

    if ( specialization() == WARLOCK_DEMONOLOGY )
      create_demonology_proc_actions();

    if ( specialization() == WARLOCK_DESTRUCTION )
      create_destruction_proc_actions();

    player_t::create_actions();
  }

  void warlock_t::create_affliction_proc_actions()
  {
  }

  void warlock_t::create_demonology_proc_actions()
  {
    // proc_actions.doom_proc = new doom_t( this );
  }

  void warlock_t::create_destruction_proc_actions()
  {
  }


  void warlock_t::create_soul_harvester_proc_actions()
  {
  }

  void warlock_t::init_special_effects()
  {
    player_t::init_special_effects();

    if ( talents.grimoire_of_sacrifice.ok() )
    {
      auto const sac_effect = new special_effect_t( this );
      sac_effect->name_str = "grimoire_of_sacrifice_effect";
      sac_effect->spell_id = talents.grimoire_of_sacrifice_buff->id();
      sac_effect->execute_action = new grimoire_of_sacrifice_damage_t( this );
      special_effects.push_back( sac_effect );

      auto cb = new dbc_proc_callback_t( this, *sac_effect );

      cb->initialize();
      cb->deactivate();

      buffs.grimoire_of_sacrifice->set_stack_change_callback( [ cb ]( buff_t*, int, int new_ ){
          if ( new_ == 1 ) cb->activate();
          else cb->deactivate();
        } );
    }
  }

  // Action Creation End

} //namespace warlock
