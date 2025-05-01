#include "simulationcraft.hpp"

#include "sc_warlock.hpp"

namespace warlock
{
  void warlock_t::init_spells()
  {
    player_t::init_spells();

    // Automatic requirement checking and relevant .inc file (/engine/dbc/generated/):
    // find_class_spell - active_spells.inc
    // find_specialization_spell - specialization_spells.inc
    // find_mastery_spell - mastery_spells.inc
    // find_talent_spell - ??
    //
    // If there is no need to check whether a spell is known by the actor, can fall back on find_spell

    // General
    warlock_base.drain_life = find_class_spell( "Drain Life" ); // Should be ID 234153
    warlock_base.corruption = find_class_spell( "Corruption" ); // Should be ID 172, DoT info is in Effect 1's trigger (146739)
    warlock_base.shadow_bolt = find_class_spell( "Shadow Bolt" ); // Should be ID 686, same for both Affliction and Demonology

    // Affliction
    warlock_base.agony = find_class_spell( "Agony" ); // Should be ID 980
    warlock_base.potent_afflictions = find_mastery_spell( WARLOCK_AFFLICTION ); // Should be ID 77215
    warlock_base.affliction_warlock = find_specialization_spell( "Affliction Warlock", WARLOCK_AFFLICTION ); // Should be ID 137043

    // Demonology
    warlock_base.hand_of_guldan = find_class_spell( "Hand of Gul'dan" ); // Should be ID 105174
    warlock_base.hog_impact = find_spell( 86040 ); // Contains impact damage data
    warlock_base.wild_imp = find_spell( 104317 ); // Contains pet summoning information
    warlock_base.fel_firebolt_2 = find_spell( 334591 ); // 20% cost reduction for Wild Imps
    warlock_base.master_demonologist = find_mastery_spell( WARLOCK_DEMONOLOGY ); // Should be ID 77219
    warlock_base.demonology_warlock = find_specialization_spell( "Demonology Warlock", WARLOCK_DEMONOLOGY ); // Should be ID 137044

    // Destruction
    warlock_base.immolate = find_specialization_spell( "Immolate" ); // Should be ID 193541
    warlock_base.immolate_old = find_spell( 348 ); // This contains the actual direct damage and cast data, but no longer appears in class_spell list
    warlock_base.immolate_dot = find_spell( 157736 ); // DoT data
    warlock_base.incinerate = find_spell( 29722 ); // Should be ID 29722 TODO: 2024-07-05 this spell was missing from the non-PTR class spell list. Fix once this comes back
    warlock_base.chaos_bolt = find_specialization_spell( "Chaos Bolt" ); // Should be ID 116858
    warlock_base.chaotic_energies = find_mastery_spell( WARLOCK_DESTRUCTION ); // Should be ID 77220
    warlock_base.destruction_warlock = find_specialization_spell( "Destruction Warlock", WARLOCK_DESTRUCTION ); // Should be ID 137046

    warlock_t::init_spells_affliction();
    warlock_t::init_spells_demonology();
    warlock_t::init_spells_destruction();

    // Talents
    talents.grimoire_of_sacrifice = find_talent_spell( talent_tree::SPECIALIZATION, "Grimoire of Sacrifice" ); // Aff/Destro only. Should be ID 108503
    talents.grimoire_of_sacrifice_buff = find_spell( 196099 ); // Buff data and RPPM
    talents.grimoire_of_sacrifice_proc = find_spell( 196100 ); // Damage data

    talents.soulburn = find_talent_spell( talent_tree::CLASS, "Soulburn" ); // Should be ID 385899
    talents.soulburn_buff = find_spell( 387626 );

    version_11_1_0_data = find_spell( 1214442 ); // For 11.1 version checking, new talent: Demonfire Infusion
  }

  void warlock_t::init_spells_affliction()
  {
    // Talents
    talents.unstable_affliction = find_talent_spell( talent_tree::SPECIALIZATION, "Unstable Affliction" ); // Should be ID 316099
    talents.unstable_affliction_2 = find_spell( 231791 ); // Soul Shard on demise
    talents.unstable_affliction_3 = find_spell( 334315 ); // +5 seconds duration
    
    // TODO: this exists in mop
    // talents.haunt = find_talent_spell( talent_tree::SPECIALIZATION, "Haunt" ); // Should be ID 48181

    // Additional Tier Set spell data

    // tier stuff goes here.
    // tier.hexflame_aff_2pc = sets->set( WARLOCK_AFFLICTION, TWW1, B2 ); // Should be ID 453643
    // tier.hexflame_aff_4pc = sets->set( WARLOCK_AFFLICTION, TWW1, B4 ); // Should be ID 453642
    // tier.umbral_lattice = find_spell( 455679 );
  }

  void warlock_t::init_spells_demonology()
  {
    // Talents
    // talents.fel_explosion = find_spell( 386609 );

    // Additional Tier Set spell data

    // tier stuff goes here.
    // tier.hexflame_demo_2pc = sets->set( WARLOCK_DEMONOLOGY, TWW1, B2 ); // Should be ID 453644
    // tier.hexflame_demo_4pc = sets->set( WARLOCK_DEMONOLOGY, TWW1, B4 ); // Should be ID 453645

    // Initialize some default values for pet spawners
    warlock_pet_list.wild_imps.set_default_duration( warlock_base.wild_imp->duration() );

    // TODO: move this to general
    // warlock_pet_list.doomguards.set_default_duration( talents.doomguard->duration() );

  }

  void warlock_t::init_spells_destruction()
  {
    // Talents
    // talents.conflagrate = find_talent_spell( talent_tree::SPECIALIZATION, "Conflagrate" ); // Should be ID 17962
    
    // Additional Tier Set spell data

    // Tier stuff goes here
    // tier.hexflame_destro_2pc = sets->set( WARLOCK_DESTRUCTION, TWW1, B2 ); // Should be ID 453647

    // Initialize some default values for pet spawners
    // TODO: move to general
    // warlock_pet_list.infernals.set_default_duration( talents.summon_infernal_main->duration() );
  }

  void warlock_t::init_base_stats()
  {
    if ( base.distance < 1.0 )
      base.distance = 30.0;

    player_t::init_base_stats();

    base.attack_power_per_strength = 0.0;
    base.attack_power_per_agility  = 0.0;
    base.spell_power_per_intellect = 1.0;

    resources.base[ RESOURCE_SOUL_SHARD ] = 5;

    if ( default_pet.empty() )
    {
      if ( specialization() == WARLOCK_AFFLICTION )
        default_pet = "imp";
      else if ( specialization() == WARLOCK_DEMONOLOGY )
        default_pet = "felguard";
      else if ( specialization() == WARLOCK_DESTRUCTION )
        default_pet = "imp";
    }
  }

  void warlock_t::create_buffs()
  {
    player_t::create_buffs();

    // Shared buffs
    buffs.grimoire_of_sacrifice = make_buff( this, "grimoire_of_sacrifice", talents.grimoire_of_sacrifice_buff )
                                      ->set_chance( 1.0 );

    buffs.soulburn = make_buff( this, "soulburn", talents.soulburn_buff );

    buffs.pet_movement = make_buff( this, "pet_movement" )->set_max_stack( 100 );

    // Affliction buffs
    create_buffs_affliction();


    // Demonology buffs
    create_buffs_demonology();

    // Destruction buffs
    create_buffs_destruction();

  }

  void warlock_t::create_buffs_affliction()
  {
    // buffs.nightfall = make_buff( this, "nightfall", talents.nightfall_buff );

  }

  void warlock_t::create_buffs_demonology()
  {
    // TODO: MOP - These things all exist
    // buffs.demonic_core = make_buff( this, "demonic_core", talents.demonic_core_buff );

    // Pet tracking buffs
    // buffs.wild_imps = make_buff( this, "wild_imps" )->set_max_stack( 40 );

  }

  void warlock_t::create_buffs_destruction()
  {
    // TODO: MOP these all exist
    // buffs.backdraft = make_buff( this, "backdraft", talents.backdraft_buff );

    // buffs.conflagration_of_chaos_cf = make_buff( this, "conflagration_of_chaos_cf", talents.conflagration_of_chaos_cf )
    //                                       ->set_default_value_from_effect( 1 );

    // buffs.conflagration_of_chaos_sb = make_buff( this, "conflagration_of_chaos_sb", talents.conflagration_of_chaos_sb )
    //                                       ->set_default_value_from_effect( 1 );


  }

  void warlock_t::create_pets()
  {
    for ( auto& pet : pet_name_list )
    {
      create_pet( pet );
    }
  }

  pet_t* warlock_t::create_pet( util::string_view pet_name, util::string_view pet_type )
  {
    pet_t* p = find_pet( pet_name );
    if ( p )
      return p;

    pet_t* summon_pet = create_main_pet( pet_name, pet_type );
    if ( summon_pet )
      return summon_pet;

    return nullptr;
  }

  void warlock_t::init_gains()
  {
    player_t::init_gains();

    if ( specialization() == WARLOCK_AFFLICTION )
      init_gains_affliction();
    if ( specialization() == WARLOCK_DEMONOLOGY )
      init_gains_demonology();
    if ( specialization() == WARLOCK_DESTRUCTION )
      init_gains_destruction();

  }

  void warlock_t::init_gains_affliction()
  {
    // gains.agony = get_gain( "agony" );
    gains.unstable_affliction_refund = get_gain( "unstable_affliction_refund" );
    gains.drain_soul = get_gain( "drain_soul" );
  }

  void warlock_t::init_gains_demonology()
  {
    // gains.soul_strike = get_gain( "soul_strike" );
  }

  void warlock_t::init_gains_destruction()
  {
    gains.immolate = get_gain( "immolate" );
    gains.immolate_crits = get_gain( "immolate_crits" );
    gains.incinerate_crits = get_gain( "incinerate_crits" );
    gains.shadowburn_refund = get_gain( "shadowburn_refund" );
  }

  void warlock_t::init_procs()
  {
    player_t::init_procs();

    if ( specialization() == WARLOCK_AFFLICTION )
      init_procs_affliction();
    if ( specialization() == WARLOCK_DEMONOLOGY )
      init_procs_demonology();
    if ( specialization() == WARLOCK_DESTRUCTION )
      init_procs_destruction();

    procs.conflagration_of_chaos_cf = get_proc( "conflagration_of_chaos_cf" );
    procs.conflagration_of_chaos_sb = get_proc( "conflagration_of_chaos_sb" );
  }

  void warlock_t::init_procs_affliction()
  {
    // procs.nightfall = get_proc( "nightfall" );
  }

  void warlock_t::init_procs_demonology()
  {
    // procs.demonic_core_dogs = get_proc( "demonic_core_dogs" );
    // procs.demonic_core_imps = get_proc( "demonic_core_imps" );
  }

  void warlock_t::init_procs_destruction()
  {
    // procs.reverse_entropy = get_proc( "reverse_entropy" );
  }

  void warlock_t::init_rng()
  {
    if ( specialization() == WARLOCK_AFFLICTION )
      init_rng_affliction();
    if ( specialization() == WARLOCK_DEMONOLOGY )
      init_rng_demonology();
    if ( specialization() == WARLOCK_DESTRUCTION )
      init_rng_destruction();

    player_t::init_rng();
  }

  void warlock_t::init_rng_affliction()
  {
    // ravenous_afflictions_rng = get_rppm( "ravenous_afflictions", talents.ravenous_afflictions );
  }

  void warlock_t::init_rng_demonology()
  {
    // jackpot_demonology_rng = get_rppm( "jackpot_demonology", tier.spliced_demo_2pc );
  }

  void warlock_t::init_rng_destruction()
  {
    // jackpot_destruction_rng = get_rppm( "jackpot_destruction", tier.spliced_destro_2pc );
  }


  void warlock_t::init_resources( bool force )
  {
    player_t::init_resources( force );

    resources.current[ RESOURCE_SOUL_SHARD ] = initial_soul_shards;
  }

  void warlock_t::init_action_list()
  {
    if ( action_list_str.empty() )
    {
      clear_action_priority_lists();

      switch ( specialization() )
      {
      case WARLOCK_AFFLICTION:
        warlock_apl::affliction( this );
        break;
      case WARLOCK_DEMONOLOGY:
        warlock_apl::demonology( this );
        break;
      case WARLOCK_DESTRUCTION:
        warlock_apl::destruction( this );
        break;
      default:
        break;
      }

      use_default_action_list = true;
    }

    player_t::init_action_list();
  }

  void warlock_t::add_rng_option( warlock_t::rng_settings_t::rng_setting_t& setting )
  {
    add_option( opt_float( "rng_" + setting.option_name, setting.setting_value ) );
  }

  void warlock_t::create_options()
  {
    player_t::create_options();

    add_option( opt_int( "soul_shards", initial_soul_shards ) );
    add_option( opt_string( "default_pet", default_pet ) );
    add_option( opt_bool( "disable_felstorm", disable_auto_felstorm ) );
    add_option( opt_bool( "normalize_destruction_mastery", normalize_destruction_mastery ) );

    add_rng_option( rng_settings.agony );
    add_rng_option( rng_settings.nightfall );
  }

  void warlock_t::combat_begin()
  {
    player_t::combat_begin();
    
    // TODO: we have an inner demon specialization to handle here...
    // if ( specialization() == WARLOCK_DEMONOLOGY && buffs.inner_demons && talents.inner_demons->ok() )
    // {
    //   timespan_t start = timespan_t::from_seconds( rng().range( talents.inner_demons->effectN( 1 ).period().total_seconds() ) );
    //   make_event( sim, start, [ this ] { buffs.inner_demons->trigger(); } );
    // }
  }

  void warlock_t::reset()
  {
    player_t::reset();

    range::for_each( sim->target_list, [ this ]( const player_t* t ) {
      if ( auto td = target_data[ t ] )
        td->reset();

      range::for_each( t->pet_list, [ this ]( const player_t* add ) {
        if ( auto td = target_data[ add ] )
          td->reset();
      } );
    } );

    warlock_pet_list.active = nullptr;
    havoc_target = nullptr;
    ua_target = nullptr;
    agony_accumulator = rng().range( 0.0, 0.99 );
    corruption_accumulator = rng().range( 0.0, 0.99 );
    wild_imp_spawns.clear();
    diabolic_ritual = as<int>( rng().range( 0, 3 ) );
  }
}
