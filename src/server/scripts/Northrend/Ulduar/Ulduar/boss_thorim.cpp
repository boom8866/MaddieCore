/*
* Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "MoveSplineInit.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "TypeContainerVisitor.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "ulduar.h"

enum Spells
{
	// Thorim
	SPELL_SHEATH_OF_LIGHTNING = 62276,
	SPELL_STORMHAMMER = 62042,
	SPELL_STORMHAMMER_SIF = 64767,
	SPELL_STORMHAMMER_BOOMERANG = 64909,
	SPELL_DEAFENING_THUNDER = 62470,
	SPELL_CHARGE_ORB = 62016,
	SPELL_SUMMON_LIGHTNING_ORB = 62391,
	SPELL_LIGHTNING_DESTRUCTION = 62393,
	SPELL_TOUCH_OF_DOMINION = 62507,
	SPELL_TOUCH_OF_DOMINION_TRIGGERED = 62565,
	SPELL_CHAIN_LIGHTNING = 62131,
	SPELL_LIGHTNING_ORB_CHARGED = 62186, // wrong duration, triggered spell should handle lightning release
	SPELL_LIGHTNING_CHARGE = 62279,
	SPELL_LIGHTNING_RELEASE = 62466,
	SPELL_LIGHTNING_PILLAR_2 = 62976, // caster high position, target low position
	SPELL_LIGHTNING_PILLAR_1 = 63238, // caster high position, target low position
	SPELL_UNBALANCING_STRIKE = 62130,
	SPELL_BERSERK_PHASE_1 = 62560,
	SPELL_BERSERK_PHASE_2 = 62555,
	SPELL_ACTIVATE_LIGHTNING_ORB_PERIODIC = 62184,

	// Credits
	SPELL_CREDIT_SIFFED = 64980,
	SPELL_CREDIT_KILL = 64985,

	// Lightning Field
	SPELL_LIGHTNING_FIELD = 64972,
	SPELL_LIGHTNING_BEAM_CHANNEL = 45537,

	// Sif
	SPELL_BLIZZARD = 62577,
	SPELL_BLINK = 62578,
	SPELL_FROSTBOLT_VOLLEY = 62580,
	SPELL_FROSTBOLT = 62583,
	SPELL_FROSTNOVA = 62597,
	SPELL_SIF_TRANSFORM = 64778,

	// Runic Colossus
	SPELL_SMASH = 62339,
	SPELL_RUNIC_BARRIER = 62338,
	SPELL_RUNIC_CHARGE = 62613,
	SPELL_RUNIC_SMASH = 62465,
	SPELL_RUNIC_SMASH_RIGHT = 62057,
	SPELL_RUNIC_SMASH_LEFT = 62058,

	// Ancient Rune Giant
	SPELL_RUNIC_FORTIFICATION = 62942,
	SPELL_RUNE_DETONATION = 62526,
	SPELL_STOMP = 62411
};

enum Phases
{
	PHASE_NULL,
	PHASE_1,
	PHASE_2
};

enum Events
{
	// Thorim
	EVENT_SAY_AGGRO_2 = 1,
	EVENT_SAY_SIF_START,
	EVENT_START_SIF_CHANNEL,
	EVENT_STORMHAMMER,
	EVENT_CHARGE_ORB,
	EVENT_SUMMON_ADDS,
	EVENT_BERSERK,
	EVENT_JUMPDOWN,
	EVENT_UNBALANCING_STRIKE,
	EVENT_CHAIN_LIGHTNING,
	EVENT_START_PERIODIC_CHARGE,
	EVENT_LIGHTNING_CHARGE,
	EVENT_ACTIVATE_LIGHTNING_FIELD,
	EVENT_CHECK_PLAYER,
	EVENT_OUTRO_1,
	EVENT_OUTRO_2,
	EVENT_OUTRO_3,

	// Runic Colossus
	EVENT_RUNIC_BARRIER,
	EVENT_SMASH,
	EVENT_RUNIC_CHARGE,
	EVENT_RUNIC_SMASH,

	// Ancient Rune Giant
	EVENT_RUNIC_FORTIFICATION,
	EVENT_STOMP,
	EVENT_RUNE_DETONATION,

	// Arena NPC
	EVENT_PRIMARY_ABILITY,
	EVENT_SECONDARY_ABILITY,
	EVENT_THIRD_ABILITY,
	EVENT_ABILITY_CHARGE,

	// Sif
	EVENT_BLINK,
	EVENT_FROST_NOVA,
	EVENT_FROSTBOLT,
	EVENT_FROSTBOLT_VOLLEY,
	EVENT_BLIZZARD
};

enum Yells
{
	// Thorim
	SAY_AGGRO_1 = 0,
	SAY_AGGRO_2 = 1,
	//SAY_SPECIAL_1                               = 2,
	SAY_SPECIAL_2 = 3,
	//SAY_SPECIAL_3                               = 4,
	SAY_JUMPDOWN = 5,
	SAY_SLAY = 6,
	SAY_BERSERK = 7,
	SAY_WIPE = 8,
	SAY_DEATH = 9,
	SAY_END_NORMAL_1 = 10,
	SAY_END_NORMAL_2 = 11,
	SAY_END_NORMAL_3 = 12,
	SAY_END_HARD_1 = 13,
	SAY_END_HARD_2 = 14,
	SAY_END_HARD_3 = 15,

	// Runic Colossus
	EMOTE_RUNIC_BARRIER = 0,

	// Ancient Rune Giant
	EMOTE_RUNIC_MIGHT = 0,

	// Sif
	SAY_SIF_START = 0,
	SAY_SIF_DESPAWN = 1,
	SAY_SIF_EVENT = 2
};

enum TrashTypes
{
	// Pre Phase Trash
	BEHEMOTH,
	MERCENARY_CAPTAIN,
	MERCENARY_SOLDIER,

	// Arena Phase Trash
	DARK_RUNE_CHAMPION,
	DARK_RUNE_WARBRINGER,
	DARK_RUNE_COMMONER,
	DARK_RUNE_EVOKER,

	// Hall Way Trash
	IRON_RING_GUARD,
	IRON_HONOR_GUARD,

	// Shared
	DARK_RUNE_ACOLYTE
};

enum PreAddSpells
{
	SPELL_ACID_BREATH = 62315,
	SPELL_SWEEP = 62316,

	SPELL_DEVASTATE = 62317,
	SPELL_HEROIC_SWIPE = 62444,
	SPELL_SUNDER_ARMOR = 57807,

	SPELL_BARBED_SHOT = 62318,
	SPELL_SHOOT = 16496,

	SPELL_RENEW = 62333,
	SPELL_GREATER_HEAL = 62334, /// 61965
	SPELL_CIRCLE_OF_HEALING = 61964,

	SPELL_HOLY_SMITE = 62335,

	SPELL_LEAP = 61934,

	SPELL_CHARGE = 32323,
	SPELL_MORTAL_STRIKE = 35054,
	SPELL_WHIRLWIND = 33500,

	SPELL_LOW_BLOW = 62326,
	SPELL_PUMMEL = 38313,

	SPELL_RUNIC_LIGHTNING = 62327,
	SPELL_RUNIC_MENDING = 62328,
	SPELL_RUNIC_SHIELD = 62321,

	SPELL_RUNIC_STRIKE = 62322,
	SPELL_AURA_OF_CELERITY = 62320,

	SPELL_IMPALE = 62331,
	SPELL_WHIRLING_TRIP = 64151,

	SPELL_CLEAVE = 42724,
	SPELL_HAMSTRING = 48639,
	SPELL_SHIELD_SMASH = 62332,
};

struct ThorimTrashInfo
{
	uint32 Type;
	uint32 Entry;
	uint32 PrimaryAbility;
	uint32 SecondaryAbility;
	uint32 ThirdAbility;
};

ThorimTrashInfo const StaticThorimTrashInfo[] =
{
	// Pre Phase
	{ BEHEMOTH, NPC_JORMUNGAR_BEHEMOTH, SPELL_ACID_BREATH, SPELL_SWEEP, 0 },
	{ MERCENARY_CAPTAIN, NPC_MERCENARY_CAPTAIN_A, SPELL_DEVASTATE, SPELL_HEROIC_SWIPE, SPELL_SUNDER_ARMOR },
	{ MERCENARY_SOLDIER, NPC_MERCENARY_SOLDIER_A, SPELL_BARBED_SHOT, SPELL_SHOOT, 0 },
	{ DARK_RUNE_ACOLYTE, NPC_DARK_RUNE_ACOLYTE_PRE, SPELL_RENEW, SPELL_GREATER_HEAL, SPELL_CIRCLE_OF_HEALING },
	{ MERCENARY_CAPTAIN, NPC_MERCENARY_CAPTAIN_H, SPELL_DEVASTATE, SPELL_HEROIC_SWIPE, SPELL_SUNDER_ARMOR },
	{ MERCENARY_SOLDIER, NPC_MERCENARY_SOLDIER_H, SPELL_BARBED_SHOT, SPELL_SHOOT, 0 },

	// Arena Phase
	{ DARK_RUNE_CHAMPION, NPC_DARK_RUNE_CHAMPION, SPELL_MORTAL_STRIKE, SPELL_WHIRLWIND, 0 },
	{ DARK_RUNE_WARBRINGER, NPC_DARK_RUNE_WARBRINGER, SPELL_RUNIC_STRIKE, 0, 0 },
	{ DARK_RUNE_EVOKER, NPC_DARK_RUNE_EVOKER, SPELL_RUNIC_LIGHTNING, SPELL_RUNIC_SHIELD, SPELL_RUNIC_MENDING },
	{ DARK_RUNE_COMMONER, NPC_DARK_RUNE_COMMONER, SPELL_LOW_BLOW, SPELL_PUMMEL, 0 },

	// Hall Way
	{ IRON_RING_GUARD, NPC_IRON_RING_GUARD, SPELL_WHIRLING_TRIP, SPELL_IMPALE, 0 },
	{ IRON_HONOR_GUARD, NPC_IRON_HONOR_GUARD, SPELL_CLEAVE, SPELL_SHIELD_SMASH, 0 },
	{ DARK_RUNE_ACOLYTE, NPC_DARK_RUNE_ACOLYTE, SPELL_RENEW, SPELL_GREATER_HEAL, 0 },
};

enum Actions
{
	ACTION_INCREASE_PREADDS_COUNT,
	ACTION_ACTIVATE_RUNIC_SMASH,
	ACTION_ACTIVATE_ADDS,
	ACTION_PILLAR_CHARGED,
	ACTION_START_HARD_MODE,
	ACTION_BERSERK
};

#define IN_ARENA(who) (who->GetPositionX() < 2181.19f && who->GetPositionY() > -299.12f)

struct SummonLocation
{
	Position pos;
	uint32 entry;
};

SummonLocation const PreAddLocations[] =
{
	{ { 2149.68f, -263.477f, 419.679f, 3.120f }, NPC_JORMUNGAR_BEHEMOTH },
	{ { 2131.31f, -271.640f, 419.840f, 2.188f }, NPC_MERCENARY_CAPTAIN_A },
	{ { 2127.24f, -259.182f, 419.974f, 5.917f }, NPC_MERCENARY_SOLDIER_A },
	{ { 2123.32f, -254.770f, 419.840f, 6.170f }, NPC_MERCENARY_SOLDIER_A },
	{ { 2120.10f, -258.990f, 419.840f, 6.250f }, NPC_MERCENARY_SOLDIER_A },
	{ { 2129.09f, -277.142f, 419.756f, 1.222f }, NPC_DARK_RUNE_ACOLYTE_PRE }
};

SummonLocation const ColossusAddLocations[] =
{
	{ { 2218.38f, -297.50f, 412.18f, 1.030f }, NPC_IRON_RING_GUARD },
	{ { 2235.07f, -297.98f, 412.18f, 1.613f }, NPC_IRON_RING_GUARD },
	{ { 2235.26f, -338.34f, 412.18f, 1.589f }, NPC_IRON_RING_GUARD },
	{ { 2217.69f, -337.39f, 412.18f, 1.241f }, NPC_IRON_RING_GUARD },
	{ { 2227.58f, -308.30f, 412.18f, 1.591f }, NPC_DARK_RUNE_ACOLYTE },
	{ { 2227.47f, -345.37f, 412.18f, 1.566f }, NPC_DARK_RUNE_ACOLYTE }
};

SummonLocation const GiantAddLocations[] =
{
	{ { 2198.05f, -428.77f, 419.95f, 6.056f }, NPC_IRON_HONOR_GUARD },
	{ { 2220.31f, -436.22f, 412.26f, 1.064f }, NPC_IRON_HONOR_GUARD },
	{ { 2158.88f, -441.73f, 438.25f, 0.127f }, NPC_IRON_HONOR_GUARD },
	{ { 2198.29f, -436.92f, 419.95f, 0.261f }, NPC_DARK_RUNE_ACOLYTE },
	{ { 2230.93f, -434.27f, 412.26f, 1.931f }, NPC_DARK_RUNE_ACOLYTE }
};

Position const SifSpawnPosition = { 2148.301f, -297.8453f, 438.3308f, 2.687807f };

enum Data
{
	ACHIEVEMENT_DONT_STAND_IN_THE_LIGHTNING = 29712972,
	ACHIEVEMENT_SIFFED = 29772978,
	ACHIEVEMENT_LOSE_YOUR_ILLUSION = 31763183,
	DATA_CHARGED_PILLAR = 1
};

enum DisplayIds
{
	THORIM_WEAPON_DISPLAY_ID = 45900
};

uint32 const LightningOrbPathSize = 8;
G3D::Vector3 const LightningOrbPath[LightningOrbPathSize] =
{
	{ 2134.889893f, -298.632996f, 438.247467f },
	{ 2134.570068f, -440.317993f, 438.247467f },
	{ 2167.820312f, -440.330261f, 438.247589f },
	{ 2213.394287f, -433.318298f, 412.665863f },
	{ 2227.766113f, -433.275818f, 412.177032f },
	{ 2227.551270f, -263.081085f, 412.176880f },
	{ 2202.208008f, -262.939270f, 412.168976f },
	{ 2182.310059f, -263.233093f, 414.739410f }
};

Position const ArenaCenter = { 2135.0f, -263.0f, 420.0f, 0.0f }; // used for trash jump calculation
Position const LightningFieldCenter = { 2135.0f, -312.5f, 438.0f, 0.0f }; // used for lightning field calculation

// p2 start at 5357

class RunicSmashExplosionEvent : public BasicEvent
{
public:
	RunicSmashExplosionEvent(Creature* owner) : _owner(owner) { }

	bool Execute(uint64 /*eventTime*/, uint32 /*updateTime*/) override
	{
		_owner->CastSpell((Unit*)nullptr, SPELL_RUNIC_SMASH);
		return true;
	}

private:
	Creature* _owner;
};

class TrashJumpEvent : public BasicEvent
{
public:
	TrashJumpEvent(Creature* owner) : _owner(owner), _stage(0) { }

	bool Execute(uint64 eventTime, uint32 /*updateTime*/) override
	{
		switch (_stage)
		{
		case 0:
			_owner->CastSpell((Unit*)nullptr, SPELL_LEAP);
			++_stage;
			_owner->m_Events.AddEvent(this, eventTime + 2000);
			return false;
		case 1:
			_owner->SetReactState(REACT_AGGRESSIVE);
			_owner->AI()->DoZoneInCombat(_owner, 200.0f);
			return true;
		default:
			break;
		}

		return true;
	}

private:
	Creature* _owner;
	uint8 _stage;
};

class LightningFieldEvent : public BasicEvent
{
public:
	LightningFieldEvent(Creature* owner) : _owner(owner) { }

	bool Execute(uint64 eventTime, uint32 /*updateTime*/) override
	{
		if (InstanceScript* instance = _owner->GetInstanceScript())
		{
			if (instance->GetBossState(BOSS_THORIM) == IN_PROGRESS)
			{
				_owner->CastSpell((Unit*)nullptr, SPELL_LIGHTNING_FIELD);
				_owner->m_Events.AddEvent(this, eventTime + 1000);
				return false;
			}
		}

		_owner->InterruptNonMeleeSpells(false);
		_owner->AI()->EnterEvadeMode();
		return true;
	}

private:
	Creature* _owner;
};

class boss_thorim : public CreatureScript
{
public:
	boss_thorim() : CreatureScript("boss_thorim") { }

	struct boss_thorimAI : public BossAI
	{
		boss_thorimAI(Creature* creature) : BossAI(creature, BOSS_THORIM)
		{
			_encounterFinished = false;
			Initialize();
		}

		void Initialize()
		{
			_killedCount = 0;
			_waveType = 0;
			_hardMode = true;
			_orbSummoned = false;
			_dontStandInTheLightning = true;
		}

		void Reset() override
		{
			if (_encounterFinished)
				return;

			_Reset();
			Initialize();

			me->SetReactState(REACT_PASSIVE);
			SetCombatMovement(false);
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);

			events.SetPhase(PHASE_NULL);

			// Respawn Mini Bosses
			for (uint8 i = DATA_RUNIC_COLOSSUS; i <= DATA_RUNE_GIANT; ++i)
				if (Creature* miniBoss = ObjectAccessor::GetCreature(*me, instance->GetGuidData(i)))
					miniBoss->Respawn(true);

			// Spawn Pre Phase Adds
			for (SummonLocation const& s : PreAddLocations)
				me->SummonCreature(s.entry, s.pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);

			if (GameObject* go = ObjectAccessor::GetGameObject(*me, instance->GetGuidData(DATA_THORIM_LEVER)))
				go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);

			// Remove trigger auras
			if (Creature* pillar = ObjectAccessor::GetCreature(*me, _activePillarGUID))
				pillar->RemoveAllAuras();

			if (Creature* controller = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_THORIM_CONTROLLER)))
				controller->RemoveAllAuras();

			_activePillarGUID.Clear();
		}

		void SetGUID(ObjectGuid guid, int32 type) override
		{
			if (type == DATA_CHARGED_PILLAR)
			{
				_activePillarGUID = guid;

				if (Creature* pillar = ObjectAccessor::GetCreature(*me, _activePillarGUID))
				{
					pillar->CastSpell(pillar, SPELL_LIGHTNING_ORB_CHARGED, true);
					pillar->CastSpell((Unit*)nullptr, SPELL_LIGHTNING_PILLAR_2);
					events.ScheduleEvent(EVENT_LIGHTNING_CHARGE, 8000, 0, PHASE_2);
				}
			}
		}

		uint32 GetData(uint32 type) const override
		{
			switch (type)
			{
			case ACHIEVEMENT_DONT_STAND_IN_THE_LIGHTNING:
				return _dontStandInTheLightning ? 1 : 0;
			case ACHIEVEMENT_LOSE_YOUR_ILLUSION:
			case DATA_THORIM_HARDMODE:
				return _hardMode ? 1 : 0;
			default:
				break;
			}

			return 0;
		}

		void KilledUnit(Unit* who) override
		{
			if (who->GetTypeId() == TYPEID_PLAYER)
				Talk(SAY_SLAY);
		}

		void SpellHit(Unit* /*caster*/, SpellInfo const* spellInfo) override
		{
			if (spellInfo->Id == SPELL_TOUCH_OF_DOMINION_TRIGGERED)
			{
				if (Creature* sif = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_SIF)))
				{
					sif->AI()->Talk(SAY_SIF_DESPAWN);
					sif->DespawnOrUnsummon(6000);
					_hardMode = false;
				}
			}
		}

		void SpellHitTarget(Unit* who, SpellInfo const* spellInfo) override
		{
			if (who->GetTypeId() == TYPEID_PLAYER && spellInfo->Id == SPELL_LIGHTNING_CHARGE)
				_dontStandInTheLightning = false;
		}

		void FinishEncounter()
		{
			if (_encounterFinished)
				return;

			_encounterFinished = true;

			DoCastAOE(SPELL_CREDIT_KILL, true); // before change faction

			me->SetReactState(REACT_PASSIVE);
			me->InterruptNonMeleeSpells(true);
			me->RemoveAllAttackers();
			me->AttackStop();
			me->setFaction(35);
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_RENAME);

			if (Creature* controller = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_THORIM_CONTROLLER)))
				controller->RemoveAllAuras();
			if (Creature* pillar = ObjectAccessor::GetCreature(*me, _activePillarGUID))
				pillar->RemoveAllAuras();

			if (_hardMode)
			{
				if (Creature* Sif = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_SIF)))
				{
					summons.Despawn(Sif);
					Sif->DespawnOrUnsummon(10000);
				}
			}

			if (_hardMode)
				me->SummonGameObject(RAID_MODE(GO_CACHE_OF_STORMS_HARDMODE_10, GO_CACHE_OF_STORMS_HARDMODE_25), 2134.948f, -286.436f, 419.5051f, 1.588249f, 0.f, 0.f, 0.7132502f, 0.7009096f, 604800);
			else
				me->SummonGameObject(RAID_MODE(GO_CACHE_OF_STORMS_10, GO_CACHE_OF_STORMS_25), 2134.948f, -286.436f, 419.5051f, 1.588249f, 0.f, 0.f, 0.7132502f, 0.7009096f, 604800);

			_JustDied();

			Talk(SAY_DEATH);
			events.ScheduleEvent(EVENT_OUTRO_1, 4000);
			events.ScheduleEvent(EVENT_OUTRO_2, _hardMode ? 8000 : 11000);
			events.ScheduleEvent(EVENT_OUTRO_3, _hardMode ? 19000 : 21000);

			me->m_Events.AddEvent(new KeeperDespawnEvent(me), me->m_Events.CalculateTime(35000));
		}

		void EnterCombat(Unit* /*who*/) override
		{
			_EnterCombat();
			Talk(SAY_AGGRO_1);

			events.SetPhase(PHASE_1);

			events.ScheduleEvent(EVENT_SAY_AGGRO_2, 9000, 0, PHASE_1);
			events.ScheduleEvent(EVENT_SAY_SIF_START, 16500, 0, PHASE_1);
			events.ScheduleEvent(EVENT_START_SIF_CHANNEL, 22500, 0, PHASE_1);

			events.ScheduleEvent(EVENT_STORMHAMMER, 40000, 0, PHASE_1);
			events.ScheduleEvent(EVENT_CHARGE_ORB, 30000, 0, PHASE_1);
			events.ScheduleEvent(EVENT_SUMMON_ADDS, 15000, 0, PHASE_1);
			events.ScheduleEvent(EVENT_BERSERK, 369000);
			events.ScheduleEvent(EVENT_CHECK_PLAYER, 10000);

			DoCast(me, SPELL_SHEATH_OF_LIGHTNING);

			if (Creature* runic = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_RUNIC_COLOSSUS)))
			{
				runic->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
				runic->AI()->DoAction(ACTION_ACTIVATE_ADDS);
			}

			if (GameObject* go = ObjectAccessor::GetGameObject(*me, instance->GetGuidData(DATA_THORIM_LEVER)))
				go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);

			// Summon Sif
			me->SummonCreature(NPC_SIF, SifSpawnPosition);
		}

		void JustSummoned(Creature* summon) override
		{
			switch (summon->GetEntry())
			{
			case NPC_LIGHTNING_ORB:
			{
				summon->SetReactState(REACT_PASSIVE);
				summon->CastSpell(summon, SPELL_LIGHTNING_DESTRUCTION, true);

				Position pos(LightningOrbPath[LightningOrbPathSize - 1].x, LightningOrbPath[LightningOrbPathSize - 1].y, LightningOrbPath[LightningOrbPathSize - 1].z);
				summon->GetMotionMaster()->MovePoint(EVENT_CHARGE_PREPATH, pos, false);

				Movement::PointsArray path(LightningOrbPath, LightningOrbPath + LightningOrbPathSize);

				Movement::MoveSplineInit init(summon);
				init.MovebyPath(path);
				init.Launch();
				break;
			}
			case NPC_DARK_RUNE_CHAMPION:
			case NPC_DARK_RUNE_WARBRINGER:
			case NPC_DARK_RUNE_EVOKER:
			case NPC_DARK_RUNE_COMMONER:
				summon->SetReactState(REACT_PASSIVE);
				summon->m_Events.AddEvent(new TrashJumpEvent(summon), summon->m_Events.CalculateTime(3000));
				break;
			case NPC_SIF:
				summon->SetReactState(REACT_PASSIVE);
				break;
			default:
				break;
			}

			BossAI::JustSummoned(summon);
		}

		void UpdateAI(uint32 diff) override
		{
			if (!UpdateVictim())
				return;

			events.Update(diff);

			if (me->HasUnitState(UNIT_STATE_CASTING))
				return;

			while (uint32 eventId = events.ExecuteEvent())
			{
				switch (eventId)
				{
				case EVENT_SAY_AGGRO_2:
					Talk(SAY_AGGRO_2);
					break;
				case EVENT_SAY_SIF_START:
					if (Creature* sif = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_SIF)))
						sif->AI()->Talk(SAY_SIF_START);
					break;
				case EVENT_START_SIF_CHANNEL:
					if (Creature* sif = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_SIF)))
						sif->CastSpell(me, SPELL_TOUCH_OF_DOMINION);
					break;
				case EVENT_STORMHAMMER:
					DoCast(SPELL_STORMHAMMER);
					events.ScheduleEvent(EVENT_STORMHAMMER, urand(15000, 20000), 0, PHASE_1);
					break;
				case EVENT_CHARGE_ORB:
					DoCastAOE(SPELL_CHARGE_ORB);
					events.ScheduleEvent(EVENT_CHARGE_ORB, urand(15000, 20000), 0, PHASE_1);
					break;
				case EVENT_SUMMON_ADDS:
					SummonWave();
					events.ScheduleEvent(EVENT_SUMMON_ADDS, _orbSummoned ? 3000 : 10000, 0, PHASE_1);
					break;
				case EVENT_JUMPDOWN:
					if (_hardMode)
						if (Creature* sif = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_SIF)))
							sif->AI()->DoAction(ACTION_START_HARD_MODE);
					me->RemoveAurasDueToSpell(SPELL_SHEATH_OF_LIGHTNING);
					me->SetReactState(REACT_AGGRESSIVE);
					SetCombatMovement(true);
					me->GetMotionMaster()->MoveJump(2134.79f, -263.03f, 419.84f, 30.0f, 20.0f);
					events.ScheduleEvent(EVENT_START_PERIODIC_CHARGE, 2000, 0, PHASE_2);
					events.ScheduleEvent(EVENT_UNBALANCING_STRIKE, 15000, 0, PHASE_2);
					events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, 20000, 0, PHASE_2);
					break;
				case EVENT_UNBALANCING_STRIKE:
					DoCastVictim(SPELL_UNBALANCING_STRIKE);
					events.ScheduleEvent(EVENT_UNBALANCING_STRIKE, urand(15000, 20000), 0, PHASE_2);
					break;
				case EVENT_CHAIN_LIGHTNING:
					if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
						DoCast(target, SPELL_CHAIN_LIGHTNING);
					events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, urand(7000, 15000), 0, PHASE_2);
					break;
				case EVENT_START_PERIODIC_CHARGE:
					if (Creature* controller = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_THORIM_CONTROLLER)))
						controller->CastSpell(controller, SPELL_ACTIVATE_LIGHTNING_ORB_PERIODIC, true);
					break;
				case EVENT_LIGHTNING_CHARGE:
					if (Creature* pillar = ObjectAccessor::GetCreature(*me, _activePillarGUID))
						DoCast(pillar, SPELL_LIGHTNING_RELEASE);
					break;
				case EVENT_BERSERK:
					if (events.IsInPhase(PHASE_1))
					{
						Talk(SAY_WIPE);
						DoCastAOE(SPELL_BERSERK_PHASE_1, true);
						DoCast(me, SPELL_SUMMON_LIGHTNING_ORB, true);
					}
					else
					{
						Talk(SAY_BERSERK);
						DoCast(me, SPELL_BERSERK_PHASE_2, true);
					}
					break;
				case EVENT_ACTIVATE_LIGHTNING_FIELD:
				{
					std::list<Creature*> triggers;
					me->GetCreatureListWithEntryInGrid(triggers, NPC_THORIM_EVENT_BUNNY, 100.0f);
					triggers.remove_if([](Creature* bunny)
					{
						if (bunny->GetPositionZ() < 430.0f)
							return true;
						return LightningFieldCenter.GetExactDist2dSq(bunny) > 1225.0f;
					});

					uint64 timer = 1000;
					for (Creature* bunny : triggers)
						bunny->m_Events.AddEvent(new LightningFieldEvent(bunny), bunny->m_Events.CalculateTime(timer += 100));

					triggers.remove_if([](Creature* bunny)
					{
						return LightningFieldCenter.GetExactDist2dSq(bunny) < 400.0f;
					});

					triggers.sort([](Creature* a, Creature* b)
					{
						return a->GetPositionX() < b->GetPositionX();
					});

					for (std::list<Creature*>::const_iterator itr = triggers.begin(); itr != triggers.end();)
					{
						std::list<Creature*>::const_iterator prev = itr++;
						if (itr != triggers.end())
							(*prev)->CastSpell(*itr, SPELL_LIGHTNING_BEAM_CHANNEL);
					}
					break;
				}
				case EVENT_CHECK_PLAYER:
					if (!me->GetMap()->GetPlayersCountExceptGMs())
						EnterEvadeMode(EVADE_REASON_OTHER); //possibly wrong?
					events.ScheduleEvent(EVENT_CHECK_PLAYER, 10000);
					break;
				case EVENT_OUTRO_1:
					Talk(_hardMode ? SAY_END_HARD_1 : SAY_END_NORMAL_1);
					if (_hardMode)
						DoCast(me, SPELL_STORMHAMMER_SIF);
					break;
				case EVENT_OUTRO_2:
					Talk(_hardMode ? SAY_END_HARD_2 : SAY_END_NORMAL_2);
					if (_hardMode)
						if (Creature* Sif = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_SIF)))
							Sif->SetStandState(UNIT_STAND_STATE_DEAD);
					break;
				case EVENT_OUTRO_3:
					Talk(_hardMode ? SAY_END_HARD_3 : SAY_END_NORMAL_3);
					break;
				default:
					break;
				}
			}

			DoMeleeAttackIfReady();
		}

		void DoAction(int32 action) override
		{
			switch (action)
			{
			case ACTION_BERSERK:
				if (events.IsInPhase(PHASE_2))
					return;

				if (!_orbSummoned)
				{
					_orbSummoned = true;
					events.RescheduleEvent(EVENT_BERSERK, 1000);
				}
				return;
			case ACTION_INCREASE_PREADDS_COUNT:
				if (++_killedCount >= 6)
				{
					// Event starts
					me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
					DoZoneInCombat(me, 250.0f);
				}
				break;
			default:
				break;
			}
		}

		void GetTrashSpawnTriggers(std::list<Creature*>& triggerList, uint32 count = 1)
		{
			me->GetCreatureListWithEntryInGrid(triggerList, NPC_THORIM_EVENT_BUNNY, 100.0f);
			triggerList.remove_if([](Creature* bunny)
			{
				if (bunny->GetPositionZ() < 430.0f)
					return true;
				return ArenaCenter.GetExactDist2dSq(bunny) < 3025.0f;
			});

			if (triggerList.empty())
				return;

			if (count == 1)
			{
				Creature* bunny = Trinity::Containers::SelectRandomContainerElement(triggerList);
				triggerList.clear();
				triggerList.push_back(bunny);
			}
			else
				Trinity::Containers::RandomResizeList(triggerList, count);
		}

		void SummonWave()
		{
			switch (_waveType)
			{
			case 0:
			{
				// Dark Rune Commoner
				std::list<Creature*> triggers;
				GetTrashSpawnTriggers(triggers, urand(5, 6));

				for (Creature* bunny : triggers)
					me->SummonCreature(StaticThorimTrashInfo[6 + 3].Entry, *bunny, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);

				++_waveType;
				break;
			}
			case 1:
				if (urand(0, 1))
				{
					// Dark Rune Champion or Dark Rune Evoker
					std::list<Creature*> triggers;
					GetTrashSpawnTriggers(triggers, urand(2, 4));

					for (Creature* bunny : triggers)
						me->SummonCreature(StaticThorimTrashInfo[6 + RAND(0, 2)].Entry, *bunny, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);
				}
				else
				{
					// Dark Rune Warbringer
					std::list<Creature*> triggers;
					GetTrashSpawnTriggers(triggers);

					for (Creature* bunny : triggers)
						me->SummonCreature(StaticThorimTrashInfo[6 + 1].Entry, *bunny, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);
				}
				--_waveType;
				break;
			}
		}

		bool CanStartPhase2(Unit* actor) const
		{
			if (actor->GetTypeId() != TYPEID_PLAYER || !me->IsWithinDistInMap(actor, 10.0f))
				return false;

			Creature* runicColossus = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_RUNIC_COLOSSUS));
			Creature* runeGiant = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_RUNE_GIANT));
			return runicColossus && !runicColossus->IsAlive() && runeGiant && !runeGiant->IsAlive();
		}

		void DamageTaken(Unit* attacker, uint32& damage) override
		{
			if (events.IsInPhase(PHASE_1) && CanStartPhase2(attacker))
			{
				Talk(SAY_JUMPDOWN);
				events.SetPhase(PHASE_2);
				events.ScheduleEvent(EVENT_JUMPDOWN, 8000);
				events.ScheduleEvent(EVENT_ACTIVATE_LIGHTNING_FIELD, 15000);
				events.RescheduleEvent(EVENT_BERSERK, 300000, 0, PHASE_2);

				if (Creature* sif = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_SIF)))
					sif->InterruptNonMeleeSpells(false);

				// Hard Mode
				if (_hardMode)
					DoCastAOE(SPELL_CREDIT_SIFFED, true);
			}
			else if (me->HealthBelowPctDamaged(1, damage))
			{
				damage = 0;
				FinishEncounter();
			}
		}

	private:
		ObjectGuid _activePillarGUID;
		uint8 _killedCount;
		uint8 _waveType;
		bool _hardMode;
		bool _encounterFinished;
		bool _orbSummoned;
		bool _dontStandInTheLightning;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetUlduarAI<boss_thorimAI>(creature);
	}
};

struct npc_thorim_trashAI : public ScriptedAI
{
	npc_thorim_trashAI(Creature* creature) : ScriptedAI(creature)
	{
		_instance = creature->GetInstanceScript();
		for (uint8 i = 0; i < 13; ++i)
			if (me->GetEntry() == StaticThorimTrashInfo[i].Entry)
				_info = &StaticThorimTrashInfo[i];

		ASSERT(_info);
	}

	struct AIHelper
	{
		/// returns heal amount of the given spell including hots
		static uint32 GetTotalHeal(SpellInfo const* spellInfo, Unit const* caster)
		{
			uint32 heal = 0;
			for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
			{
				if (spellInfo->Effects[i].IsEffect(SPELL_EFFECT_HEAL))
					heal += spellInfo->Effects[i].CalcValue(caster);

				if (spellInfo->Effects[i].IsEffect(SPELL_EFFECT_APPLY_AURA) && spellInfo->Effects[i].IsAura(SPELL_AURA_PERIODIC_HEAL))
					heal += spellInfo->GetMaxTicks() * spellInfo->Effects[i].CalcValue(caster);
			}
			return heal;
		}

		/// returns heal amount of the given spell including hots
		static uint32 GetTotalHeal(uint32 spellId, Unit const* caster)
		{
			if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId))
				return GetTotalHeal(spellInfo, caster);
			return 0;
		}

		/// returns remaining heal amount on given target
		static uint32 GetRemainingHealOn(Unit* target)
		{
			uint32 heal = 0;
			Unit::AuraApplicationMap const& auras = target->GetAppliedAuras();
			for (Unit::AuraApplicationMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
			{
				Aura const* aura = itr->second->GetBase();

				for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
				{
					if (AuraEffect const* aurEff = aura->GetEffect(i))
					{
						if (aurEff->GetAuraType() == SPELL_AURA_PERIODIC_HEAL)
							heal += aurEff->GetAmount() * (aurEff->GetTotalTicks() - aurEff->GetTickNumber());
					}
				}
			}
			return heal;
		}

		class MostHPMissingInRange
		{
		public:
			MostHPMissingInRange(Unit const* referer, float range, uint32 hp, uint32 exclAura = 0, bool exclSelf = false)
				: _referer(referer), _range(range), _hp(hp), _exclAura(exclAura), _exclSelf(exclSelf) { }

			bool operator()(Unit* u)
			{
				if (_exclSelf && u == _referer)
					return false;

				if (_exclAura && u->HasAura(sSpellMgr->GetSpellIdForDifficulty(_exclAura, _referer)))
					return false;

				if ((u->GetHealth() + GetRemainingHealOn(u) + _hp) > u->GetMaxHealth())
					return false;

				uint32 missingHP = u->GetMaxHealth() - u->GetHealth();
				if (u->IsAlive() && _referer->IsFriendlyTo(u) && _referer->IsWithinDistInMap(u, _range) && missingHP > _hp)
				{
					_hp = missingHP;
					return true;
				}

				return false;
			}

		private:
			Unit const* _referer;
			float _range;
			uint32 _hp;
			uint32 _exclAura;
			bool _exclSelf;
		};

		static Unit* GetUnitWithMostMissingHp(SpellInfo const* spellInfo, Unit* caster)
		{
			float range = spellInfo->GetMaxRange(false);
			uint32 heal = GetTotalHeal(spellInfo, caster);

			Unit* target = nullptr;
			MostHPMissingInRange checker(caster, range, heal);
			Trinity::UnitLastSearcher<MostHPMissingInRange> searcher(caster, target, checker);
			caster->VisitNearbyObject(range, searcher);

			return target;
		}

		static Unit* GetHealTarget(uint32 spellId, Unit* caster)
		{
			Unit* healTarget = nullptr;

			if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId))
			{
				if (!spellInfo->HasAttribute(SPELL_ATTR1_CANT_TARGET_SELF) && !roll_chance_f(caster->GetHealthPct()) && !((caster->GetHealth() + GetRemainingHealOn(caster) + GetTotalHeal(spellInfo, caster)) > caster->GetMaxHealth()))
					healTarget = caster;
				else
					healTarget = GetUnitWithMostMissingHp(spellInfo, caster);
			}

			return healTarget;
		}
	};

	bool UseAbility(uint32 spellId)
	{
		Unit* target = nullptr;
		if (AIHelper::GetTotalHeal(spellId, me))
			target = AIHelper::GetHealTarget(spellId, me);
		else
			target = me->GetVictim();

		if (target)
		{
			if (_info->Type == MERCENARY_SOLDIER)
			{
				bool allowMove = true;
				if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId))
				{
					if (me->IsInRange(target, spellInfo->GetMinRange(false), spellInfo->GetMaxRange(false))
						&& me->IsWithinLOSInMap(target))
						allowMove = false;
				}
				SetCombatMovement(allowMove);
			}

			DoCast(target, spellId);
			return true;
		}

		return false;
	}

	void UpdateAI(uint32 diff) final override
	{
		if (!UpdateVictim())
			return;

		_events.Update(diff);

		if (me->HasUnitState(UNIT_STATE_CASTING))
			return;

		while (uint32 eventId = _events.ExecuteEvent())
		{
			ExecuteEvent(eventId);
		}

		if (_info->Type == DARK_RUNE_ACOLYTE)
			DoSpellAttackIfReady(SPELL_HOLY_SMITE);
		else
			DoMeleeAttackIfReady();
	}

	virtual void ExecuteEvent(uint32 eventId) = 0;

protected:
	InstanceScript* _instance;
	EventMap _events;

	ThorimTrashInfo const* _info = nullptr;
};

class npc_thorim_pre_phase : public CreatureScript
{
public:
	npc_thorim_pre_phase() : CreatureScript("npc_thorim_pre_phase") { }

	struct npc_thorim_pre_phaseAI : public npc_thorim_trashAI
	{
		npc_thorim_pre_phaseAI(Creature* creature) : npc_thorim_trashAI(creature)
		{
			me->setActive(true); // prevent grid unload
		}

		void Reset()
		{
			_events.Reset();
			if (_info->PrimaryAbility)
				_events.ScheduleEvent(EVENT_PRIMARY_ABILITY, urand(3000, 6000));
			if (_info->SecondaryAbility)
				_events.ScheduleEvent(EVENT_SECONDARY_ABILITY, _info->SecondaryAbility == SPELL_SHOOT ? 2000 : urand(12000, 15000));
			if (_info->ThirdAbility)
				_events.ScheduleEvent(EVENT_THIRD_ABILITY, urand(6000, 8000));
			if (_info->Type == MERCENARY_SOLDIER)
				SetCombatMovement(false);
		}

		void JustDied(Unit* /*victim*/)
		{
			if (Creature* thorim = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(BOSS_THORIM)))
				thorim->AI()->DoAction(ACTION_INCREASE_PREADDS_COUNT);
		}

		void DamageTaken(Unit* attacker, uint32& damage) override
		{
			if (!attacker->GetAffectingPlayer())
				damage = 0;
		}

		void ExecuteEvent(uint32 eventId) override
		{
			switch (eventId)
			{
			case EVENT_PRIMARY_ABILITY:
				if (UseAbility(_info->PrimaryAbility))
					_events.ScheduleEvent(eventId, urand(15000, 20000));
				else
					_events.ScheduleEvent(eventId, 1000);
				break;
			case EVENT_SECONDARY_ABILITY:
				if (UseAbility(_info->SecondaryAbility))
					_events.ScheduleEvent(eventId, _info->SecondaryAbility == SPELL_SHOOT ? 2000 : urand(4000, 8000));
				else
					_events.ScheduleEvent(eventId, 1000);
				break;
			case EVENT_THIRD_ABILITY:
				if (UseAbility(_info->ThirdAbility))
					_events.ScheduleEvent(eventId, urand(6000, 8000));
				else
					_events.ScheduleEvent(eventId, 1000);
				break;
			default:
				break;
			}
		}
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetUlduarAI<npc_thorim_pre_phaseAI>(creature);
	}
};

class npc_thorim_arena_phase : public CreatureScript
{
public:
	npc_thorim_arena_phase() : CreatureScript("npc_thorim_arena_phase") { }

	struct npc_thorim_arena_phaseAI : public npc_thorim_trashAI
	{
		npc_thorim_arena_phaseAI(Creature* creature) : npc_thorim_trashAI(creature)
		{
			switch (_info->Type)
			{
			case DARK_RUNE_CHAMPION:
			case DARK_RUNE_WARBRINGER:
			case DARK_RUNE_COMMONER:
			case DARK_RUNE_EVOKER:
				_isInArena = true;
				break;
			case DARK_RUNE_ACOLYTE:
				_isInArena = (_info->Entry == NPC_DARK_RUNE_ACOLYTE_PRE);
				break;
			default:
				_isInArena = false;
				break;
			}
		}

		bool CanAIAttack(Unit const* who) const override
		{
			return _isInArena == IN_ARENA(who);
		}

		void Reset() override
		{
			_events.Reset();
			if (_info->PrimaryAbility)
				_events.ScheduleEvent(EVENT_PRIMARY_ABILITY, urand(3000, 6000));
			if (_info->SecondaryAbility)
				_events.ScheduleEvent(EVENT_SECONDARY_ABILITY, urand(7000, 9000));
			if (_info->ThirdAbility)
				_events.ScheduleEvent(EVENT_THIRD_ABILITY, urand(6000, 8000));
			if (_info->Type == DARK_RUNE_CHAMPION)
				_events.ScheduleEvent(EVENT_ABILITY_CHARGE, 8000);
		}

		void EnterCombat(Unit* /*who*/)
		{
			if (_info->Type == DARK_RUNE_WARBRINGER)
				DoCast(me, SPELL_AURA_OF_CELERITY);

			if (!_isInArena)
				if (Creature* colossus = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(DATA_RUNIC_COLOSSUS)))
					colossus->AI()->DoAction(ACTION_ACTIVATE_RUNIC_SMASH);
		}

		void EnterEvadeMode(EvadeReason why) override
		{
			// this should only happen if theres no alive player in the arena -> summon orb
			// might be called by mind control release or controllers death?
			if (Creature* thorim = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(BOSS_THORIM)))
				thorim->AI()->DoAction(ACTION_BERSERK);
			ScriptedAI::EnterEvadeMode(why);
		}

		void ExecuteEvent(uint32 eventId) override
		{
			switch (eventId)
			{
			case EVENT_PRIMARY_ABILITY:
				if (UseAbility(_info->PrimaryAbility))
					_events.ScheduleEvent(eventId, urand(3000, 6000));
				else
					_events.ScheduleEvent(eventId, 1000);
				break;
			case EVENT_SECONDARY_ABILITY:
				if (UseAbility(_info->SecondaryAbility))
					_events.ScheduleEvent(eventId, urand(12000, 16000));
				else
					_events.ScheduleEvent(eventId, 1000);
				break;
			case EVENT_THIRD_ABILITY:
				if (UseAbility(_info->ThirdAbility))
					_events.ScheduleEvent(eventId, urand(6000, 8000));
				else
					_events.ScheduleEvent(eventId, 1000);
				break;
			case EVENT_ABILITY_CHARGE:
				if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, [this](Unit* unit) { return unit->GetTypeId() == TYPEID_PLAYER && unit->IsInRange(me, 8.0f, 25.0f); }))
					DoCast(target, SPELL_CHARGE);
				_events.ScheduleEvent(eventId, 12000);
				break;
			default:
				break;
			}
		}

	private:
		bool _isInArena;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetUlduarAI<npc_thorim_arena_phaseAI>(creature);
	}
};

struct npc_thorim_minibossAI : public ScriptedAI
{
	npc_thorim_minibossAI(Creature* creature) : ScriptedAI(creature), _summons(me)
	{
		_instance = creature->GetInstanceScript();
		creature->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
	}

	void JustSummoned(Creature* summon) final override
	{
		_summons.Summon(summon);
	}

	void SummonedCreatureDespawn(Creature* summon) final override
	{
		_summons.Despawn(summon);
	}

	void DoAction(int32 action) override
	{
		if (action == ACTION_ACTIVATE_ADDS)
		{
			for (ObjectGuid const& guid : _summons)
				if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
					summon->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
		}
	}

protected:
	InstanceScript* _instance;
	EventMap _events;
	SummonList _summons;
};

class npc_runic_colossus : public CreatureScript
{
public:
	npc_runic_colossus() : CreatureScript("npc_runic_colossus") { }

	struct npc_runic_colossusAI : public npc_thorim_minibossAI
	{
		npc_runic_colossusAI(Creature* creature) : npc_thorim_minibossAI(creature) { }

		void Reset() override
		{
			_runicActive = false;

			_events.Reset();

			// Runed Door closed
			_instance->HandleGameObject(_instance->GetGuidData(DATA_RUNIC_DOOR), false);

			// Spawn trashes
			_summons.DespawnAll();
			for (SummonLocation const& s : ColossusAddLocations)
				me->SummonCreature(s.entry, s.pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);
		}

		void MoveInLineOfSight(Unit* /*who*/) override
		{
			// don't enter combat
		}

		void DoAction(int32 action) override
		{
			npc_thorim_minibossAI::DoAction(action);

			if (_runicActive)
				return;

			if (action == ACTION_ACTIVATE_RUNIC_SMASH)
			{
				_runicActive = true;
				_events.ScheduleEvent(EVENT_RUNIC_SMASH, 7000);
			}
		}

		void JustDied(Unit* /*victim*/) override
		{
			// Runed Door opened
			_instance->HandleGameObject(_instance->GetGuidData(DATA_RUNIC_DOOR), true);

			if (Creature* thorim = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(BOSS_THORIM)))
				thorim->AI()->Talk(SAY_SPECIAL_2);

			if (Creature* giant = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(DATA_RUNE_GIANT)))
			{
				giant->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
				giant->AI()->DoAction(ACTION_ACTIVATE_ADDS);
			}
		}

		void EnterCombat(Unit* /*who*/) override
		{
			DoZoneInCombat();
			_events.Reset();
			_events.ScheduleEvent(EVENT_RUNIC_BARRIER, urand(12000, 15000));
			_events.ScheduleEvent(EVENT_SMASH, urand(15000, 18000));
			_events.ScheduleEvent(EVENT_RUNIC_CHARGE, urand(20000, 24000));
			me->InterruptNonMeleeSpells(true);
		}

		void UpdateAI(uint32 diff) override
		{
			_events.Update(diff);

			if (me->HasUnitState(UNIT_STATE_CASTING))
				return;

			while (uint32 eventId = _events.ExecuteEvent())
			{
				switch (eventId)
				{
				case EVENT_RUNIC_BARRIER:
					Talk(EMOTE_RUNIC_BARRIER);
					DoCast(me, SPELL_RUNIC_BARRIER);
					_events.ScheduleEvent(eventId, urand(35000, 45000));
					break;
				case EVENT_SMASH:
					DoCast(me, SPELL_SMASH);
					_events.ScheduleEvent(eventId, urand(15000, 18000));
					break;
				case EVENT_RUNIC_CHARGE:
					if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, [this](Unit* unit) { return unit->GetTypeId() == TYPEID_PLAYER && unit->IsInRange(me, 8.0f, 40.0f); }))
						DoCast(target, SPELL_RUNIC_CHARGE);
					_events.ScheduleEvent(eventId, 20000);
					break;
				case EVENT_RUNIC_SMASH:
					DoCast(me, RAND(SPELL_RUNIC_SMASH_LEFT, SPELL_RUNIC_SMASH_RIGHT));
					_events.ScheduleEvent(eventId, 6000);
					break;
				default:
					break;
				}
			}

			if (!UpdateVictim())
				return;

			DoMeleeAttackIfReady();
		}

	private:
		bool _runicActive;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetUlduarAI<npc_runic_colossusAI>(creature);
	}
};

class npc_ancient_rune_giant : public CreatureScript
{
public:
	npc_ancient_rune_giant() : CreatureScript("npc_ancient_rune_giant") { }

	struct npc_ancient_rune_giantAI : public npc_thorim_minibossAI
	{
		npc_ancient_rune_giantAI(Creature* creature) : npc_thorim_minibossAI(creature) { }

		void Reset() override
		{
			_events.Reset();

			// Stone Door closed
			_instance->HandleGameObject(_instance->GetGuidData(DATA_STONE_DOOR), false);

			// Spawn trashes
			_summons.DespawnAll();
			for (SummonLocation const& s : GiantAddLocations)
				me->SummonCreature(s.entry, s.pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);
		}

		void EnterCombat(Unit* /*who*/) override
		{
			DoZoneInCombat();
			_events.Reset();
			_events.ScheduleEvent(EVENT_RUNIC_FORTIFICATION, 1);
			_events.ScheduleEvent(EVENT_STOMP, urand(10000, 12000));
			_events.ScheduleEvent(EVENT_RUNE_DETONATION, 25000);
		}

		void JustDied(Unit* /*victim*/) override
		{
			// Stone Door opened
			_instance->HandleGameObject(_instance->GetGuidData(DATA_STONE_DOOR), true);
		}

		void UpdateAI(uint32 diff) override
		{
			if (!UpdateVictim())
				return;

			_events.Update(diff);

			if (me->HasUnitState(UNIT_STATE_CASTING))
				return;

			while (uint32 eventId = _events.ExecuteEvent())
			{
				switch (eventId)
				{
				case EVENT_RUNIC_FORTIFICATION:
					Talk(EMOTE_RUNIC_MIGHT);
					DoCastAOE(SPELL_RUNIC_FORTIFICATION);
					break;
				case EVENT_STOMP:
					DoCastAOE(SPELL_STOMP);
					_events.ScheduleEvent(eventId, urand(10000, 12000));
					break;
				case EVENT_RUNE_DETONATION:
					if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 60.0f, true))
						DoCast(target, SPELL_RUNE_DETONATION);
					_events.ScheduleEvent(eventId, urand(10000, 12000));
					break;
				default:
					break;
				}
			}

			DoMeleeAttackIfReady();
		}
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetUlduarAI<npc_ancient_rune_giantAI>(creature);
	}
};

class npc_sif : public CreatureScript
{
public:
	npc_sif() : CreatureScript("npc_sif") { }

	struct npc_sifAI : public ScriptedAI
	{
		npc_sifAI(Creature* creature) : ScriptedAI(creature)
		{
			SetCombatMovement(false);
			_instance = creature->GetInstanceScript();
		}

		void Reset() override
		{
			_events.Reset();
		}

		void SpellHit(Unit* /*caster*/, SpellInfo const* spellInfo) override
		{
			if (spellInfo->Id == SPELL_STORMHAMMER_SIF)
			{
				if (Creature* blizzard = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(DATA_SIF_BLIZZARD)))
					blizzard->RemoveAllAuras();

				me->InterruptSpell(CURRENT_GENERIC_SPELL);
				me->SetReactState(REACT_PASSIVE);
				me->AttackStop();
			}
		}

		void DoAction(int32 action) override
		{
			if (action == ACTION_START_HARD_MODE)
			{
				me->SetReactState(REACT_AGGRESSIVE);
				DoZoneInCombat(me, 250.0f);
				Talk(SAY_SIF_EVENT);
				_events.Reset();
				_events.ScheduleEvent(EVENT_FROSTBOLT, 2000);
				_events.ScheduleEvent(EVENT_FROSTBOLT_VOLLEY, 15000);
				_events.ScheduleEvent(EVENT_BLINK, urand(20000, 25000));
				_events.ScheduleEvent(EVENT_BLIZZARD, 30000);
			}
		}

		void UpdateAI(uint32 diff) override
		{
			if (!UpdateVictim())
				return;

			_events.Update(diff);

			if (me->HasUnitState(UNIT_STATE_CASTING))
				return;

			while (uint32 eventId = _events.ExecuteEvent())
			{
				switch (eventId)
				{
				case EVENT_BLINK:
					if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
						DoCast(target, SPELL_BLINK);
					_events.ScheduleEvent(EVENT_FROST_NOVA, 0);
					_events.ScheduleEvent(eventId, urand(20000, 25000));
					return;
				case EVENT_FROST_NOVA:
					DoCastAOE(SPELL_FROSTNOVA);
					return;
				case EVENT_FROSTBOLT:
					if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
						DoCast(target, SPELL_FROSTBOLT);
					_events.ScheduleEvent(eventId, 2000);
					return;
				case EVENT_FROSTBOLT_VOLLEY:
					DoCastAOE(SPELL_FROSTBOLT_VOLLEY);
					_events.ScheduleEvent(eventId, urand(15000, 20000));
					return;
				case EVENT_BLIZZARD:
					DoCastAOE(SPELL_BLIZZARD);
					_events.ScheduleEvent(eventId, urand(35000, 45000));
					return;
				default:
					break;
				}
			}

			// no melee attack
		}

	private:
		EventMap _events;
		InstanceScript* _instance;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetUlduarAI<npc_sifAI>(creature);
	}
};

class HeightPositionCheck
{
public:
	HeightPositionCheck(bool ret) : _ret(ret) { }

	bool operator() (WorldObject* obj) const
	{
		return obj->GetPositionZ() > 425.0f == _ret;
	}

private:
	bool _ret;
};

// 62577 - Blizzard
// 62603 - Blizzard
class spell_thorim_blizzard : public SpellScriptLoader
{
public:
	spell_thorim_blizzard() : SpellScriptLoader("spell_thorim_blizzard") { }

	class spell_thorim_blizzard_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_thorim_blizzard_SpellScript);

		void SelectTarget(std::list<WorldObject*>& targets)
		{
			targets.clear();

			if (Creature* blizzard = ObjectAccessor::GetCreature(*GetCaster(), _instance->GetGuidData(DATA_SIF_BLIZZARD)))
				targets.push_back(blizzard);
		}

		bool Load() override
		{
			_instance = GetCaster()->GetInstanceScript();
			return _instance != nullptr;
		}

		InstanceScript* _instance;

		void Register() override
		{
			OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_thorim_blizzard_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_thorim_blizzard_SpellScript();
	}
};

// 62576 - Blizzard
// 62602 - Blizzard
class spell_thorim_blizzard_effect : public SpellScriptLoader
{
public:
	spell_thorim_blizzard_effect() : SpellScriptLoader("spell_thorim_blizzard_effect") { }

	class spell_thorim_blizzard_effect_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_thorim_blizzard_effect_AuraScript);

		bool CheckAreaTarget(Unit* target)
		{
			/// @todo: fix this for all dynobj auras
			if (target != GetOwner())
			{
				// check if not stacking aura already on target
				// this one prevents overriding auras periodically by 2 near area aura owners
				Unit::AuraApplicationMap const& auraMap = target->GetAppliedAuras();
				for (Unit::AuraApplicationMap::const_iterator iter = auraMap.begin(); iter != auraMap.end(); ++iter)
				{
					Aura const* aura = iter->second->GetBase();
					if (GetId() == aura->GetId() && GetOwner() != aura->GetOwner() /*!GetAura()->CanStackWith(aura)*/)
						return false;
				}
			}
			return true;
		}

		void Register() override
		{
			DoCheckAreaTarget += AuraCheckAreaTargetFn(spell_thorim_blizzard_effect_AuraScript::CheckAreaTarget);
		}
	};

	AuraScript* GetAuraScript() const override
	{
		return new spell_thorim_blizzard_effect_AuraScript();
	}
};

// 62580, 62604 - Frostbolt Volley
class spell_thorim_frostbolt_volley : public SpellScriptLoader
{
public:
	spell_thorim_frostbolt_volley() : SpellScriptLoader("spell_thorim_frostbolt_volley") { }

	class spell_thorim_frostbolt_volley_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_thorim_frostbolt_volley_SpellScript);

		void FilterTargets(std::list<WorldObject*>& unitList)
		{
			unitList.remove_if(PlayerOrPetCheck());
		}

		void Register() override
		{
			OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_thorim_frostbolt_volley_SpellScript::FilterTargets, EFFECT_ALL, TARGET_UNIT_SRC_AREA_ENEMY);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_thorim_frostbolt_volley_SpellScript();
	}
};

// 62016 - Charge Orb
class spell_thorim_charge_orb : public SpellScriptLoader
{
public:
	spell_thorim_charge_orb() : SpellScriptLoader("spell_thorim_charge_orb") { }

	class spell_thorim_charge_orb_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_thorim_charge_orb_SpellScript);

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(SPELL_LIGHTNING_PILLAR_1))
				return false;
			return true;
		}

		void FilterTargets(std::list<WorldObject*>& targets)
		{
			targets.remove_if(HeightPositionCheck(false));

			if (targets.empty())
				return;

			WorldObject* target = Trinity::Containers::SelectRandomContainerElement(targets);
			targets.clear();
			targets.push_back(target);
		}

		void HandleScript()
		{
			if (Unit* target = GetHitUnit())
				target->CastSpell((Unit*)nullptr, SPELL_LIGHTNING_PILLAR_1, true);
		}

		void Register() override
		{
			OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_thorim_charge_orb_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
			AfterHit += SpellHitFn(spell_thorim_charge_orb_SpellScript::HandleScript);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_thorim_charge_orb_SpellScript();
	}
};

// 62466 - Lightning Charge
class spell_thorim_lightning_charge : public SpellScriptLoader
{
public:
	spell_thorim_lightning_charge() : SpellScriptLoader("spell_thorim_lightning_charge") { }

	class spell_thorim_lightning_charge_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_thorim_lightning_charge_SpellScript);

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(SPELL_LIGHTNING_CHARGE))
				return false;
			return true;
		}

		void HandleFocus()
		{
			/// @workaround: focus target is not working because spell is triggered and instant
			if (Creature* creature = GetCaster()->ToCreature())
				creature->FocusTarget(GetSpell(), GetExplTargetWorldObject());
		}

		void HandleCharge()
		{
			GetCaster()->CastSpell(GetCaster(), SPELL_LIGHTNING_CHARGE);
		}

		void Register() override
		{
			BeforeCast += SpellCastFn(spell_thorim_lightning_charge_SpellScript::HandleFocus);
			AfterCast += SpellCastFn(spell_thorim_lightning_charge_SpellScript::HandleCharge);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_thorim_lightning_charge_SpellScript();
	}
};

// 61934 - Leap
class spell_thorim_arena_leap : public SpellScriptLoader
{
public:
	spell_thorim_arena_leap() : SpellScriptLoader("spell_thorim_arena_leap") { }

	class spell_thorim_arena_leap_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_thorim_arena_leap_SpellScript);

		bool Load() override
		{
			return GetCaster()->GetTypeId() == TYPEID_UNIT;
		}

		void HandleScript(SpellEffIndex /*effIndex*/)
		{
			if (GetHitDest())
				GetCaster()->ToCreature()->SetHomePosition(*GetHitDest());
		}

		void Register() override
		{
			OnEffectHitTarget += SpellEffectFn(spell_thorim_arena_leap_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_JUMP_DEST);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_thorim_arena_leap_SpellScript();
	}
};

// 62042 - Stormhammer
class spell_thorim_stormhammer : public SpellScriptLoader
{
public:
	spell_thorim_stormhammer() : SpellScriptLoader("spell_thorim_stormhammer") { }

	class spell_thorim_stormhammer_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_thorim_stormhammer_SpellScript);

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(SPELL_STORMHAMMER_BOOMERANG)
				|| !sSpellMgr->GetSpellInfo(SPELL_DEAFENING_THUNDER))
				return false;
			return true;
		}

		void FilterTargets(std::list<WorldObject*>& targets)
		{
			targets.remove_if([](WorldObject* target)
			{
				return !IN_ARENA(target);
			});

			if (targets.empty())
			{
				FinishCast(SPELL_FAILED_NO_VALID_TARGETS);
				return;
			}

			WorldObject* target = Trinity::Containers::SelectRandomContainerElement(targets);
			targets.clear();
			targets.push_back(target);
		}

		void HandleScript(SpellEffIndex /*effIndex*/)
		{
			if (Unit* target = GetHitUnit())
			{
				target->CastSpell(target, SPELL_DEAFENING_THUNDER, true);
				target->CastSpell(GetCaster(), SPELL_STORMHAMMER_BOOMERANG, true);
			}
		}

		void LoseHammer()
		{
			GetCaster()->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, 0);
		}

		void Register() override
		{
			OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_thorim_stormhammer_SpellScript::FilterTargets, EFFECT_ALL, TARGET_UNIT_SRC_AREA_ENEMY);
			AfterCast += SpellCastFn(spell_thorim_stormhammer_SpellScript::LoseHammer);
			OnEffectHitTarget += SpellEffectFn(spell_thorim_stormhammer_SpellScript::HandleScript, EFFECT_2, SPELL_EFFECT_SCRIPT_EFFECT);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_thorim_stormhammer_SpellScript();
	}
};

// 64767 - Stormhammer
class spell_thorim_stormhammer_sif : public SpellScriptLoader
{
public:
	spell_thorim_stormhammer_sif() : SpellScriptLoader("spell_thorim_stormhammer_sif") { }

	class spell_thorim_stormhammer_sif_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_thorim_stormhammer_sif_SpellScript);

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(SPELL_STORMHAMMER_BOOMERANG)
				|| !sSpellMgr->GetSpellInfo(SPELL_SIF_TRANSFORM))
				return false;
			return true;
		}

		void HandleScript(SpellEffIndex /*effIndex*/)
		{
			if (Unit* target = GetHitUnit())
			{
				target->CastSpell(GetCaster(), SPELL_STORMHAMMER_BOOMERANG, true);
				target->CastSpell(target, SPELL_SIF_TRANSFORM, true);
			}
		}

		void LoseHammer()
		{
			GetCaster()->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, 0);
		}

		void Register() override
		{
			AfterCast += SpellCastFn(spell_thorim_stormhammer_sif_SpellScript::LoseHammer);
			OnEffectHitTarget += SpellEffectFn(spell_thorim_stormhammer_sif_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_thorim_stormhammer_sif_SpellScript();
	}
};

// 64909 - Stormhammer
class spell_thorim_stormhammer_boomerang : public SpellScriptLoader
{
public:
	spell_thorim_stormhammer_boomerang() : SpellScriptLoader("spell_thorim_stormhammer_boomerang") { }

	class spell_thorim_stormhammer_boomerang_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_thorim_stormhammer_boomerang_SpellScript);

		void RecoverHammer(SpellEffIndex /*effIndex*/)
		{
			if (Unit* target = GetHitUnit())
				target->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, THORIM_WEAPON_DISPLAY_ID);
		}

		void Register() override
		{
			OnEffectHitTarget += SpellEffectFn(spell_thorim_stormhammer_boomerang_SpellScript::RecoverHammer, EFFECT_0, SPELL_EFFECT_DUMMY);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_thorim_stormhammer_boomerang_SpellScript();
	}
};

// 62057, 62058 - Runic Smash
class spell_thorim_runic_smash : public SpellScriptLoader
{
public:
	spell_thorim_runic_smash() : SpellScriptLoader("spell_thorim_runic_smash") { }

	class spell_thorim_runic_smash_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_thorim_runic_smash_SpellScript);

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(SPELL_RUNIC_SMASH))
				return false;
			return true;
		}

		void HandleScript(SpellEffIndex effIndex)
		{
			PreventHitDefaultEffect(effIndex);

			std::list<Creature*> triggers;
			GetCaster()->GetCreatureListWithEntryInGrid(triggers, GetSpellInfo()->Id == SPELL_RUNIC_SMASH_LEFT ? NPC_GOLEM_LEFT_HAND_BUNNY : NPC_GOLEM_RIGHT_HAND_BUNNY, 150.0f);
			for (Creature* bunny : triggers)
			{
				float dist = GetCaster()->GetExactDist(bunny);
				bunny->m_Events.AddEvent(new RunicSmashExplosionEvent(bunny), bunny->m_Events.CalculateTime(uint64(dist * 30)));
			};
		}

		void Register() override
		{
			OnEffectHitTarget += SpellEffectFn(spell_thorim_runic_smash_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_TRIGGER_SPELL);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_thorim_runic_smash_SpellScript();
	}
};

class UpperOrbCheck
{
public:
	UpperOrbCheck() : _check(true) { }

	bool operator() (Creature* target) const
	{
		return target->GetEntry() == NPC_THUNDER_ORB && _check(target);
	}

private:
	HeightPositionCheck _check;
};

// 62184 - Activate Lightning Orb Periodic
class spell_thorim_activate_lightning_orb_periodic : public SpellScriptLoader
{
public:
	spell_thorim_activate_lightning_orb_periodic() : SpellScriptLoader("spell_thorim_activate_lightning_orb_periodic") { }

	class spell_thorim_activate_lightning_orb_periodic_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_thorim_activate_lightning_orb_periodic_AuraScript);

		InstanceScript* instance;

		void PeriodicTick(AuraEffect const* /*aurEff*/)
		{
			PreventDefaultAction();

			Unit* caster = GetCaster();
			std::list<Creature*> triggers;

			UpperOrbCheck check;
			Trinity::CreatureListSearcher<UpperOrbCheck> searcher(caster, triggers, check);
			caster->VisitNearbyGridObject(100.f, searcher);

			if (!triggers.empty())
			{
				Creature* target = Trinity::Containers::SelectRandomContainerElement(triggers);
				if (Creature* Thorim = ObjectAccessor::GetCreature(*caster, instance->GetGuidData(BOSS_THORIM)))
					Thorim->AI()->SetGUID(target->GetGUID(), DATA_CHARGED_PILLAR);
			}
		}

		bool Load() override
		{
			instance = nullptr;
			if (Unit* caster = GetCaster())
				instance = caster->GetInstanceScript();

			return instance != nullptr;
		}

		void Register() override
		{
			OnEffectPeriodic += AuraEffectPeriodicFn(spell_thorim_activate_lightning_orb_periodic_AuraScript::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
		}
	};

	AuraScript* GetAuraScript() const override
	{
		return new spell_thorim_activate_lightning_orb_periodic_AuraScript();
	}
};

// 62331, 62418 - Impale
class spell_iron_ring_guard_impale : public SpellScriptLoader
{
public:
	spell_iron_ring_guard_impale() : SpellScriptLoader("spell_iron_ring_guard_impale") { }

	class spell_iron_ring_guard_impale_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_iron_ring_guard_impale_AuraScript);

		void PeriodicTick(AuraEffect const* /*aurEff*/)
		{
			if (GetTarget()->HealthAbovePct(GetSpellInfo()->Effects[EFFECT_1].CalcValue()))
			{
				Remove(AURA_REMOVE_BY_ENEMY_SPELL);
				PreventDefaultAction();
			}
		}

		void Register() override
		{
			OnEffectPeriodic += AuraEffectPeriodicFn(spell_iron_ring_guard_impale_AuraScript::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
		}
	};

	AuraScript* GetAuraScript() const override
	{
		return new spell_iron_ring_guard_impale_AuraScript();
	}
};

class achievement_dont_stand_in_the_lightning : public AchievementCriteriaScript
{
public:
	achievement_dont_stand_in_the_lightning() : AchievementCriteriaScript("achievement_dont_stand_in_the_lightning") { }

	bool OnCheck(Player* /*source*/, Unit* target) override
	{
		return target && target->GetAI()->GetData(ACHIEVEMENT_DONT_STAND_IN_THE_LIGHTNING);
	}
};

class achievement_lose_your_illusion : public AchievementCriteriaScript
{
public:
	achievement_lose_your_illusion() : AchievementCriteriaScript("achievement_lose_your_illusion") { }

	bool OnCheck(Player* /*source*/, Unit* target) override
	{
		return target && target->GetAI()->GetData(ACHIEVEMENT_LOSE_YOUR_ILLUSION);
	}
};

class condition_thorim_arena_leap : public ConditionScript
{
public:
	condition_thorim_arena_leap() : ConditionScript("condition_thorim_arena_leap"), _check(false) { }

	bool OnConditionCheck(Condition* condition, ConditionSourceInfo& sourceInfo) override
	{
		WorldObject* target = sourceInfo.mConditionTargets[condition->ConditionTarget];
		InstanceScript* instance = target->GetInstanceScript();

		if (!instance)
			return false;

		return target->GetGUID() != instance->GetGuidData(DATA_SIF_BLIZZARD) && _check(target);
	}

	

private:
	HeightPositionCheck _check;
};

void AddSC_boss_thorim()
{
	new boss_thorim();
	new npc_thorim_pre_phase();
	new npc_thorim_arena_phase();
	new npc_runic_colossus();
	new npc_ancient_rune_giant();
	new npc_sif();
	new spell_thorim_blizzard();
	new spell_thorim_blizzard_effect();
	new spell_thorim_frostbolt_volley();
	new spell_thorim_charge_orb();
	new spell_thorim_lightning_charge();
	new spell_thorim_stormhammer();
	new spell_thorim_stormhammer_sif();
	new spell_thorim_stormhammer_boomerang();
	new spell_thorim_arena_leap();
	new spell_thorim_runic_smash();
	new spell_thorim_activate_lightning_orb_periodic();
	new spell_iron_ring_guard_impale();
	new achievement_dont_stand_in_the_lightning();
	new achievement_lose_your_illusion();
	new condition_thorim_arena_leap();
}
