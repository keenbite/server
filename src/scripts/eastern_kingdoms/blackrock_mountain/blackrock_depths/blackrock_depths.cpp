/**
 * ScriptDev3 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013 ScriptDev2 <http://www.scriptdev2.com/>
 * Copyright (C) 2014-2025 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

/**
 * ScriptData
 * SDName:      Blackrock_Depths
 * SD%Complete: 80
 * SDComment:   Quest support: 4001, 4134, 4201, 4322, 4342, 7604, 9015.
 * SDCategory:  Blackrock Depths
 * EndScriptData
 */

/**
 * ContentData
 * go_bar_beer_keg
 * go_shadowforge_brazier
 * go_relic_coffer_door
 * at_shadowforge_brige
 * at_ring_of_law
 * npc_grimstone
 * npc_phalanx
 * npc_mistress_nagmara
 * npc_rocknot
 * npc_marshal_windsor
 * npc_dughal_stormwing
 * npc_tobias_seecher
 * npc_hurley_blackbreath
 * boss_doomrel
 * boss_plugger_spazzring
 * npc_kharan_mighthammer
 * go_bar_ale_mug
 * npc_ironhand_guardian
 * EndContentData
 */

#include "blackrock_depths.h"
#include "escort_ai.h"

/*######
## go_bar_beer_keg
######*/

struct go_bar_beer_keg : public GameObjectScript
{
    go_bar_beer_keg() : GameObjectScript("go_bar_beer_keg") {}

    bool OnUse(Player* /*pPlayer*/, GameObject* pGo) override
    {
        if (ScriptedInstance* pInstance = (ScriptedInstance*)pGo->GetInstanceData())
        {
            if (pInstance->GetData(TYPE_HURLEY) == IN_PROGRESS || pInstance->GetData(TYPE_HURLEY) == DONE) // GOs despawning on use, this check should never be true but this is proper to have it there
            {
                return false;
            }
            else
                // Every time we set the event to SPECIAL, the instance script increments the number of broken kegs, capping at 3
                pInstance->SetData(TYPE_HURLEY, SPECIAL);
        }
        return false;
    }
};

/*######
## go_shadowforge_brazier
######*/

struct go_shadowforge_brazier : public GameObjectScript
{
    go_shadowforge_brazier() : GameObjectScript("go_shadowforge_brazier") {}

    bool OnUse(Player* /*pPlayer*/, GameObject* pGo) override
    {
        if (ScriptedInstance* pInstance = (ScriptedInstance*)pGo->GetInstanceData())
        {
            if (pInstance->GetData(TYPE_LYCEUM) == IN_PROGRESS)
            {
                pInstance->SetData(TYPE_LYCEUM, DONE);
            }
            else
            {
                pInstance->SetData(TYPE_LYCEUM, IN_PROGRESS);
            }
        }
        return false;
    }
};

/*######
## go_relic_coffer_door
######*/

struct go_relic_coffer_door : public GameObjectScript
{
    go_relic_coffer_door() : GameObjectScript("go_relic_coffer_door") {}

    bool OnUse(Player* /*pPlayer*/, GameObject* pGo) override
    {
        if (ScriptedInstance* pInstance = (ScriptedInstance*)pGo->GetInstanceData())
        {
            // check if the event is already done
            if (pInstance->GetData(TYPE_VAULT) != DONE && pInstance->GetData(TYPE_VAULT) != IN_PROGRESS)
            {
                pInstance->SetData(TYPE_VAULT, SPECIAL);
            }
        }

        return false;
    }
};

 /*######
## at_shadowforge_bridge
######*/

static const float aGuardSpawnPositions[2][4] =
{
    {642.3660f, -274.5155f, -43.10918f, 0.4712389f},                // First guard spawn position
    {740.1137f, -283.3448f, -42.75082f, 2.8623400f}                 // Second guard spawn position
};

enum
{
    SAY_GUARD_AGGRO                    = -1230043
};

struct at_shadowforge_bridge : public AreaTriggerScript
{
    at_shadowforge_bridge() : AreaTriggerScript("at_shadowforge_bridge") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* pAt) override
    {
        if (instance_blackrock_depths* pInstance = (instance_blackrock_depths*)pPlayer->GetInstanceData())
        {
            if (pPlayer->isGameMaster() || pInstance->GetData(TYPE_BRIDGE) == DONE)
            {
                return false;
            }

            Creature* pPyromancer = pInstance->GetSingleCreatureFromStorage(NPC_LOREGRAIN);

            if (!pPyromancer)
            {
                return false;
            }

            if (Creature* pMasterGuard = pPyromancer->SummonCreature(NPC_ANVILRAGE_GUARDMAN, aGuardSpawnPositions[0][0], aGuardSpawnPositions[0][1], aGuardSpawnPositions[0][2], aGuardSpawnPositions[0][3], TEMPSPAWN_DEAD_DESPAWN, 0))
            {
                pMasterGuard->SetWalk(false);
                pMasterGuard->GetMotionMaster()->MoveWaypoint();
                DoDisplayText(pMasterGuard, SAY_GUARD_AGGRO, pPlayer);
                float fX, fY, fZ;
                pPlayer->GetContactPoint(pMasterGuard, fX, fY, fZ);
                pMasterGuard->GetMotionMaster()->MovePoint(1,fX, fY, fZ);

                if (Creature* pSlaveGuard = pPyromancer->SummonCreature(NPC_ANVILRAGE_GUARDMAN, aGuardSpawnPositions[1][0], aGuardSpawnPositions[1][1], aGuardSpawnPositions[1][2], aGuardSpawnPositions[1][3], TEMPSPAWN_DEAD_DESPAWN, 0))
                {
                    pSlaveGuard->GetMotionMaster()->MoveFollow(pMasterGuard, 2.0f, 0);
                }
            }
            pInstance->SetData(TYPE_BRIDGE, DONE);
        }
        return false;
    }
};


/*######
## at_ring_of_law
######*/

/* Notes about this event:
 * Visual: Npc Grimstone should use some visual spell when appear/ disappear / opening/ closing doors
 * Texts: The texts and their positions need confirmation
 * Event timer might also need adjustment
 * Quest-Event: This needs to be clearified - there is some suggestion, that Theldren&Adds also might come as first wave.
 */

enum
{
    SAY_START_1                     = -1230004,
    SAY_START_2                     = -1230005,
    SAY_OPEN_EAST_GATE              = -1230006,
    SAY_SUMMON_BOSS_1               = -1230007,
    SAY_SUMMON_BOSS_2               = -1230008,
    SAY_OPEN_NORTH_GATE             = -1230009,

    NPC_GRIMSTONE                   = 10096,
    DATA_BANNER_BEFORE_EVENT        = 5,

    // 4 or 6 in total? 1+2+1 / 2+2+2 / 3+3. Depending on this, code should be changed.
    MAX_THELDREN_ADDS               = 4,
    MAX_POSSIBLE_THELDREN_ADDS      = 8,

    SPELL_SUMMON_THELRIN_DND        = 27517,
    // Other spells used by Grimstone
    SPELL_ASHCROMBES_TELEPORT_A     = 15742,
    SPELL_ASHCROMBES_TELEPORT_B     = 6422,
    SPELL_ARENA_FLASH_A             = 15737,
    SPELL_ARENA_FLASH_B             = 15739,
    SPELL_ARENA_FLASH_C             = 15740,
    SPELL_ARENA_FLASH_D             = 15741,

    QUEST_THE_CHALLENGE             = 9015,
    NPC_THELDREN_QUEST_CREDIT       = 16166,
};

enum SpawnPosition
{
    POS_EAST                        = 0,
    POS_NORTH                       = 1,
    POS_GRIMSTONE                   = 2,
};

static const float aSpawnPositions[3][4] =
{
    {608.960f, -235.322f, -53.907f, 1.857f},                // Ring mob spawn position
    {644.300f, -175.989f, -53.739f, 3.418f},                // Ring boss spawn position
    {625.559f, -205.618f, -52.735f, 2.609f}                 // Grimstone spawn position
};

static const uint32 aGladiator[MAX_POSSIBLE_THELDREN_ADDS] = {NPC_LEFTY, NPC_ROTFANG, NPC_SNOKH, NPC_MALGEN, NPC_KORV, NPC_REZZNIK, NPC_VAJASHNI, NPC_VOLIDA};
static const uint32 aRingMob[2][6] = {
    {NPC_WORM, NPC_STINGER, NPC_SCREECHER, NPC_THUNDERSNOUT, NPC_CREEPER, NPC_BEETLE}, // NPC template entry
    {4, 2, 5, 3, 3, 7}                                                                 // Number of NPCs per wave (two waves)
};
static const uint32 aRingBoss[] = {NPC_GOROSH, NPC_GRIZZLE, NPC_EVISCERATOR, NPC_OKTHOR, NPC_ANUBSHIAH, NPC_HEDRUM};

enum Phases
{
    PHASE_MOBS                      = 0,
    PHASE_BOSS                      = 2,
    PHASE_GLADIATORS                = 3,
};

struct at_ring_of_law : public AreaTriggerScript
{
    at_ring_of_law() : AreaTriggerScript("at_ring_of_law") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* pAt) override
    {
        if (instance_blackrock_depths* pInstance = (instance_blackrock_depths*)pPlayer->GetInstanceData())
        {
            if (pInstance->GetData(TYPE_RING_OF_LAW) == IN_PROGRESS || pInstance->GetData(TYPE_RING_OF_LAW) == DONE || pInstance->GetData(TYPE_RING_OF_LAW) == SPECIAL)
            {
                return false;
            }

            if (pPlayer->isGameMaster())
            {
                return false;
            }

            pInstance->SetData(TYPE_RING_OF_LAW, pInstance->GetData(TYPE_RING_OF_LAW) == DATA_BANNER_BEFORE_EVENT ? SPECIAL : IN_PROGRESS);

            pPlayer->SummonCreature(NPC_GRIMSTONE, aSpawnPositions[POS_GRIMSTONE][0], aSpawnPositions[POS_GRIMSTONE][1], aSpawnPositions[POS_GRIMSTONE][2], aSpawnPositions[POS_GRIMSTONE][3], TEMPSPAWN_DEAD_DESPAWN, 0);
            pInstance->SetData(TYPE_SIGNAL, pAt->id);
            pInstance->SetArenaCenterCoords(pAt->x, pAt->y, pAt->z);

            return false;
        }
        return false;
    }
};

/*######
## npc_grimstone
######*/

struct npc_grimstone : public CreatureScript
{
    npc_grimstone() : CreatureScript("npc_grimstone") {}

    struct npc_grimstoneAI : public npc_escortAI
    {
        npc_grimstoneAI(Creature* pCreature) : npc_escortAI(pCreature)
        {
            m_pInstance = (instance_blackrock_depths*)pCreature->GetInstanceData();
            m_uiMobSpawnId = urand(0, 5);
            // Select MAX_THELDREN_ADDS(4) random adds for Theldren encounter
            uint8 uiCount = 0;
            for (uint8 i = 0; i < MAX_POSSIBLE_THELDREN_ADDS && uiCount < MAX_THELDREN_ADDS; ++i)
            {
                if (urand(0, 1) || i >= MAX_POSSIBLE_THELDREN_ADDS - MAX_THELDREN_ADDS + uiCount)
                {
                    m_uiGladiatorId[uiCount] = aGladiator[i];
                    ++uiCount;
                }
            }

            Reset();
        }

        instance_blackrock_depths* m_pInstance;

        uint8 m_uiEventPhase;
        uint32 m_uiEventTimer;

        uint8 m_uiMobSpawnId;
        uint8 m_uiAliveSummonedMob;

        Phases m_uiPhase;

        uint32 m_uiGladiatorId[MAX_THELDREN_ADDS];

        GuidList m_lSummonedGUIDList;
        GuidSet m_lArenaCrowd;

        void Reset() override
        {
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

            m_uiEventTimer    = 1000;
            m_uiEventPhase    = 0;
            m_uiAliveSummonedMob  = 0;

            m_uiPhase = PHASE_MOBS;
        }

        void JustSummoned(Creature* pSummoned) override
        {
            if (!m_pInstance)
            {
                return;
            }

            // Ring mob or boss summoned
            float fX, fY, fZ;
            float fcX, fcY, fcZ;
            m_pInstance->GetArenaCenterCoords(fX, fY, fZ);
            m_creature->GetRandomPoint(fX, fY, fZ, 10.0f, fcX, fcY, fcZ);
            pSummoned->GetMotionMaster()->MovePoint(1, fcX, fcY, fcZ);

            ++m_uiAliveSummonedMob;
            m_lSummonedGUIDList.push_back(pSummoned->GetObjectGuid());
        }

        void DoChallengeQuestCredit()
        {
            Map::PlayerList const& PlayerList = m_creature->GetMap()->GetPlayers();

            for (Map::PlayerList::const_iterator itr = PlayerList.begin(); itr != PlayerList.end(); ++itr)
            {
                Player* pPlayer = itr->getSource();
                if (pPlayer && pPlayer->GetQuestStatus(QUEST_THE_CHALLENGE) == QUEST_STATUS_INCOMPLETE)
                {
                    pPlayer->KilledMonsterCredit(NPC_THELDREN_QUEST_CREDIT);
                }
            }
        }

        void SummonedCreatureJustDied(Creature* /*pSummoned*/) override
        {
            --m_uiAliveSummonedMob;

            switch (m_uiPhase)
            {
                case PHASE_MOBS:                                // Ring mob killed
                case PHASE_BOSS:                                // Ring boss killed
                    // One Boss
                    if (m_uiAliveSummonedMob == 0)
                    {
                        m_uiEventTimer = 5000;
                    }
                    break;
                case PHASE_GLADIATORS:                          // Theldren and his band killed
                    // Adds + Theldren
                    if (m_uiAliveSummonedMob == 0)
                    {
                        m_uiEventTimer = 5000;
                        DoChallengeQuestCredit();
                    }
                    break;
            }
        }

        void SummonRingMob(uint32 uiEntry, uint8 uiNpcPerWave, SpawnPosition uiPosition)
        {
            float fX, fY, fZ;
            for (uint8 i = 0; i < uiNpcPerWave; ++i)
            {
                m_creature->GetRandomPoint(aSpawnPositions[uiPosition][0], aSpawnPositions[uiPosition][1], aSpawnPositions[uiPosition][2], 2.0f, fX, fY, fZ);
                m_creature->SummonCreature(uiEntry, fX, fY, fZ, 0, TEMPSPAWN_DEAD_DESPAWN, 0);
            }
        }

        void WaypointReached(uint32 uiPointId) override
        {
            switch (uiPointId)
            {
                case 0:                                         // Middle reached first time
                    DoScriptText(SAY_START_1, m_creature);
                    SetEscortPaused(true);
                    m_uiEventTimer = 5000;
                    break;
                case 1:                                         // Reached wall again
                    DoScriptText(SAY_OPEN_EAST_GATE, m_creature);
                    SetEscortPaused(true);
                    m_uiEventTimer = 5000;
                    break;
                case 2:                                         // walking along the wall, while door opened
                    SetEscortPaused(true);
                    break;
                case 3:                                         // Middle reached second time
                    DoScriptText(SAY_SUMMON_BOSS_1, m_creature);
                    break;
                case 4:                                         // Reached North Gate
                    DoScriptText(SAY_OPEN_NORTH_GATE, m_creature);
                    SetEscortPaused(true);
                    m_uiEventTimer = 5000;
                    break;
                case 5:
                    if (m_pInstance)
                    {
                        m_pInstance->SetData(TYPE_RING_OF_LAW, DONE);
                        // debug_log("SD2: npc_grimstone: event reached end and set complete.");
                    }
                    break;
            }
        }

        void UpdateEscortAI(const uint32 uiDiff) override
        {
            if (!m_pInstance)
            {
                return;
            }

            if (m_pInstance->GetData(TYPE_RING_OF_LAW) == FAIL)
            {
                // Reset Doors
                if (m_uiEventPhase >= 10)                       // North Gate is opened
                {
                    m_pInstance->DoUseDoorOrButton(GO_ARENA_2);
                    m_pInstance->DoUseDoorOrButton(GO_ARENA_4);
                }
                else if (m_uiEventPhase >= 4)                   // East Gate is opened
                {
                    m_pInstance->DoUseDoorOrButton(GO_ARENA_1);
                    m_pInstance->DoUseDoorOrButton(GO_ARENA_4);
                }

                // Despawn Summoned Mobs
                for (GuidList::const_iterator itr = m_lSummonedGUIDList.begin(); itr != m_lSummonedGUIDList.end(); ++itr)
                {
                    if (Creature* pSummoned = m_creature->GetMap()->GetCreature(*itr))
                    {
                        pSummoned->ForcedDespawn();
                    }
                }
                m_lSummonedGUIDList.clear();

                // Despawn NPC
                m_creature->ForcedDespawn();
                return;
            }

            if (m_uiEventTimer)
            {
                if (m_uiEventTimer <= uiDiff)
                {
                    switch (m_uiEventPhase)
                    {
                        case 0:
                            // Shortly after spawn, start walking
                            DoCastSpellIfCan(m_creature, SPELL_ASHCROMBES_TELEPORT_A, CAST_TRIGGERED);
                            DoScriptText(SAY_START_2, m_creature);
                            m_pInstance->DoUseDoorOrButton(GO_ARENA_4);
                            // Some of the NPCs in the crowd do cheer emote at event start
                            // we randomly select 25% of the NPCs to do this
                            m_pInstance->GetArenaCrowdGuid(m_lArenaCrowd);
                            for (GuidSet::const_iterator itr = m_lArenaCrowd.begin(); itr != m_lArenaCrowd.end(); ++itr)
                            {
                                if (Creature* pSpectator = m_creature->GetMap()->GetCreature(*itr))
                                {
                                    if (urand(0, 3) < 1)
                                    {
                                        pSpectator->HandleEmote(EMOTE_ONESHOT_CHEER);
                                    }
                                }
                            }
                            Start(false);
                            SetEscortPaused(false);
                            m_uiEventTimer = 0;
                            break;
                        case 1:
                            // Start walking towards wall
                            SetEscortPaused(false);
                            m_uiEventTimer = 0;
                            break;
                        case 2:
                            m_uiEventTimer = 2000;
                            break;
                        case 3:
                            // Open East Gate
                            DoCastSpellIfCan(m_creature, SPELL_ARENA_FLASH_A, CAST_TRIGGERED);
                            DoCastSpellIfCan(m_creature, SPELL_ARENA_FLASH_B, CAST_TRIGGERED);
                            m_pInstance->DoUseDoorOrButton(GO_ARENA_1);
                            m_uiEventTimer = 3000;
                            break;
                        case 4:
                            // timer for teleport out spell which has 2000 ms cast time
                            DoCastSpellIfCan(m_creature, SPELL_ASHCROMBES_TELEPORT_B, CAST_TRIGGERED);
                            m_uiEventTimer = 2500;
                            break;
                        case 5:
                            m_creature->SetVisibility(VISIBILITY_OFF);
                            SetEscortPaused(false);
                            // Summon Ring Mob(s)
                            SummonRingMob(aRingMob[0][m_uiMobSpawnId], aRingMob[1][m_uiMobSpawnId], POS_EAST);
                            m_uiEventTimer = 16000;
                            break;
                        case 7:
                            // Summon Ring Mob(s)
                            SummonRingMob(aRingMob[0][m_uiMobSpawnId], aRingMob[1][m_uiMobSpawnId], POS_EAST);
                            m_uiEventTimer = 0;
                            break;
                        case 8:
                            // Summoned Mobs are dead, continue event
                            DoScriptText(SAY_SUMMON_BOSS_2, m_creature);
                            m_creature->SetVisibility(VISIBILITY_ON);
                            DoCastSpellIfCan(m_creature, SPELL_ASHCROMBES_TELEPORT_A, CAST_TRIGGERED);
                            m_pInstance->DoUseDoorOrButton(GO_ARENA_1);
                            SetEscortPaused(false);
                            m_uiEventTimer = 0;
                            break;
                        case 9:
                            // Open North Gate
                            DoCastSpellIfCan(m_creature, SPELL_ARENA_FLASH_C, CAST_TRIGGERED);
                            DoCastSpellIfCan(m_creature, SPELL_ARENA_FLASH_D, CAST_TRIGGERED);
                            m_pInstance->DoUseDoorOrButton(GO_ARENA_2);
                            m_uiEventTimer = 5000;
                            break;
                        case 10:
                            // timer for teleport out spell which has 2000 ms cast time
                            DoCastSpellIfCan(m_creature, SPELL_ASHCROMBES_TELEPORT_B, CAST_TRIGGERED);
                            m_uiEventTimer = 2500;
                            break;
                        case 11:
                            // Summon Boss
                            m_creature->SetVisibility(VISIBILITY_OFF);
                            // If banner summoned after start, then summon Thelden after the creatures are dead
                            if (m_pInstance->GetData(TYPE_RING_OF_LAW) == SPECIAL && m_uiPhase == PHASE_MOBS)
                            {
                                m_uiPhase = PHASE_GLADIATORS;
                                SummonRingMob(NPC_THELDREN, 1, POS_NORTH);
                                for (uint8 i = 0; i < MAX_THELDREN_ADDS; ++i)
                                {
                                    SummonRingMob(m_uiGladiatorId[i], 1, POS_NORTH);
                                }
                            }
                            else
                            {
                                m_uiPhase = PHASE_BOSS;
                                SummonRingMob(aRingBoss[urand(0, 5)], 1, POS_NORTH);
                            }
                            m_uiEventTimer = 0;
                            break;
                        case 12:
                            // Boss dead
                            m_lSummonedGUIDList.clear();
                            m_pInstance->DoUseDoorOrButton(GO_ARENA_2);
                            m_pInstance->DoUseDoorOrButton(GO_ARENA_3);
                            m_pInstance->DoUseDoorOrButton(GO_ARENA_4);
                            SetEscortPaused(false);
                            m_uiEventTimer = 0;
                            break;
                    }
                    ++m_uiEventPhase;
                }
                else
                {
                    m_uiEventTimer -= uiDiff;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_grimstoneAI(pCreature);
    }
};

struct spell_banner_of_provocation : public SpellScript
{
    spell_banner_of_provocation() : SpellScript("spell_banner_of_provocation") {}

    bool EffectDummy(Unit* /*pCaster*/, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Object* pCreatureTarget, ObjectGuid /*originalCasterGuid*/) override
    {
        if (uiSpellId == SPELL_SUMMON_THELRIN_DND && uiEffIndex == EFFECT_INDEX_1)
        {
            InstanceData* pInstance = pCreatureTarget->ToCreature()->GetInstanceData();
            if (pInstance && pInstance->GetData(TYPE_RING_OF_LAW) != DONE && pInstance->GetData(TYPE_RING_OF_LAW) != SPECIAL)
            {
                pInstance->SetData(TYPE_RING_OF_LAW, pInstance->GetData(TYPE_RING_OF_LAW) == IN_PROGRESS ? uint32(SPECIAL) : uint32(DATA_BANNER_BEFORE_EVENT));
            }

            return true;
        }
        return false;
    }
};


/*######
+## npc_phalanx
+######*/

enum
{
    YELL_PHALANX_AGGRO    = -1230040,

    SPELL_THUNDERCLAP     = 15588,
    SPELL_MIGHTY_BLOW     = 14099,
    SPELL_FIREBALL_VOLLEY = 15285,
};

struct npc_phalanxAI : public npc_escortAI
{
    npc_phalanxAI(Creature* pCreature) : npc_escortAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();

        Reset();
    }

    ScriptedInstance* m_pInstance;

    float m_fKeepDoorOrientation;
    uint32 uiThunderclapTimer;
    uint32 uiMightyBlowTimer;
    uint32 uiFireballVolleyTimer;
    uint32 uiCallPatrolTimer;

    void Reset() override
    {
        // If reset after an fight, it means Phalanx has already started moving (if not already reached door)
        // so we made him restart right before reaching the door to guard it (again)
        if (HasEscortState(STATE_ESCORT_ESCORTING) || HasEscortState(STATE_ESCORT_PAUSED))
        {
            SetCurrentWaypoint(1);
            SetEscortPaused(false);
        }

        m_fKeepDoorOrientation = 2.06059f;
        uiThunderclapTimer     = 0;
        uiMightyBlowTimer      = 0;
        uiFireballVolleyTimer  = 0;
        uiCallPatrolTimer      = 0;
    }

    void Aggro(Unit* /*pWho*/) override
    {
        uiThunderclapTimer     = 12000;
        uiMightyBlowTimer      = 15000;
        uiFireballVolleyTimer  = 1;
    }

    void WaypointReached(uint32 uiPointId) override
    {
        if (!m_pInstance)
        {
            return;
        }

        switch (uiPointId)
        {
            case 0:
                DoScriptText(YELL_PHALANX_AGGRO, m_creature);
                break;
            case 1:
                SetEscortPaused(true);
                // There are two ways of activating Phalanx: completing Rocknot event, making Phalanx hostile to anyone
                // killing Plugger making Phalanx hostile to Horde (do not ask why)
                // In the later case, Phalanx should also spawn the bar patrol with some delay and only then set the Plugger
                // event to DONE. In the weird case where Plugger was previously killed (event == DONE) but Phalanx is reactivated
                //  (like on reset after a wipe), do not spawn the patrol again
                if (m_pInstance->GetData(TYPE_PLUGGER) == DONE || m_pInstance->GetData(TYPE_PLUGGER) == IN_PROGRESS)
                {
                    m_creature->SetFactionTemporary(FACTION_IRONFORGE, TEMPFACTION_NONE);
                    if (m_pInstance->GetData(TYPE_PLUGGER) == IN_PROGRESS)
                    {
                        uiCallPatrolTimer = 10000;
                        m_pInstance->SetData(TYPE_PLUGGER, DONE);
                    }
                }
                else
                {
                    m_creature->SetFactionTemporary(FACTION_DARK_IRON, TEMPFACTION_NONE);
                }

                m_creature->SetFacingTo(m_fKeepDoorOrientation);
                break;
        }
    }

    void UpdateEscortAI(const uint32 uiDiff) override
    {
        if (!m_pInstance)
        {
            return;
        }

        if (uiCallPatrolTimer)
        {
            if (uiCallPatrolTimer < uiDiff && m_pInstance->GetData(TYPE_BAR) != DONE)
            {
                m_pInstance->SetData(TYPE_BAR, IN_PROGRESS);
                uiCallPatrolTimer = 0;
            }
            else
            {
                uiCallPatrolTimer -= uiDiff;
            }
        }

        // Combat check
        if (m_creature->SelectHostileTarget() && m_creature->getVictim())
        {
            if (uiThunderclapTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_THUNDERCLAP) == CAST_OK)
                {
                    uiThunderclapTimer = 10000;
                }
            }
            else
            {
                uiThunderclapTimer -= uiDiff;
            }

            if (uiMightyBlowTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_MIGHTY_BLOW) == CAST_OK)
                {
                    uiMightyBlowTimer = 10000;
                }
            }
            else
            {
                uiMightyBlowTimer -= uiDiff;
            }

            if (m_creature->GetHealthPercent() < 51.0f)
            {
                if (uiFireballVolleyTimer < uiDiff)
                {
                    if (DoCastSpellIfCan(m_creature, SPELL_FIREBALL_VOLLEY) == CAST_OK)
                    {
                        uiFireballVolleyTimer = 15000;
                    }
                }
                else
                {
                    uiFireballVolleyTimer -= uiDiff;
                }
            }

            DoMeleeAttackIfReady();
        }
    }
};

struct npc_phalanx : public CreatureScript
{
    npc_phalanx() : CreatureScript("npc_phalanx") {}


    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_phalanxAI(pCreature);
    }
};


/*######
## npc_mistress_nagmara
######*/

enum
{
    GOSSIP_ITEM_NAGMARA         = -3230003,
    GOSSIP_ID_NAGMARA           = 2727,
    GOSSIP_ID_NAGMARA_2         = 2729,
    SPELL_POTION_LOVE           = 14928,
    SPELL_NAGMARA_ROCKNOT       = 15064,

    SAY_NAGMARA_1               = -1230066,
    SAY_NAGMARA_2               = -1230067,
    TEXTEMOTE_NAGMARA           = -1230068,
    TEXTEMOTE_ROCKNOT           = -1230069,

    QUEST_POTION_LOVE           = 4201
};

struct npc_mistress_nagmaraAI : public ScriptedAI
{
    npc_mistress_nagmaraAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;
    Creature* pRocknot;

    uint8 m_uiPhase;
    uint32 m_uiPhaseTimer;

    void Reset() override
    {
        m_uiPhase = 0;
        m_uiPhaseTimer = 0;
    }

    void DoPotionOfLoveIfCan()
    {
        if (!m_pInstance)
        {
            return;
        }

        pRocknot = m_pInstance->GetSingleCreatureFromStorage(NPC_PRIVATE_ROCKNOT);
        if (!pRocknot)
        {
            return;
        }

        m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
        pRocknot->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);

        m_creature->GetMotionMaster()->MoveIdle();
        m_creature->GetMotionMaster()->MoveFollow(pRocknot, 2.0f, 0);
        m_uiPhase = 1;
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (m_uiPhaseTimer)
        {
            if (m_uiPhaseTimer <= uiDiff)
            {
                m_uiPhaseTimer = 0;
            }
            else
            {
                m_uiPhaseTimer -= uiDiff;
                return;
            }
        }

        if (!pRocknot)
        {
            return;
        }

        switch (m_uiPhase)
        {
            case 0:     // Phase 0 : Nagmara patrols in the bar to serve patrons or is following Rocknot passively
                break;
            case 1:     // Phase 1 : Nagmara is moving towards Rocknot
                if (m_creature->IsWithinDist2d(pRocknot->GetPositionX(), pRocknot->GetPositionY(), 5.0f))
                {
                    m_creature->GetMotionMaster()->MoveIdle();
                    m_creature->SetFacingToObject(pRocknot);
                    pRocknot->SetFacingToObject(m_creature);
                    DoScriptText(SAY_NAGMARA_1, m_creature);
                    m_uiPhase++;
                    m_uiPhaseTimer = 5000;
                }
                else
                {
                    m_creature->GetMotionMaster()->MoveFollow(pRocknot, 2.0f, 0);
                }
                break;
            case 2:     // Phase 2 : Nagmara is "seducing" Rocknot
                DoScriptText(SAY_NAGMARA_2, m_creature);
                m_uiPhaseTimer = 4000;
                m_uiPhase++;
                break;
            case 3:     // Phase 3: Nagmara give potion to Rocknot and Rocknot escort AI will handle the next part of the event
                if (DoCastSpellIfCan(m_creature, SPELL_POTION_LOVE) == CAST_OK)
                {
                    m_uiPhase = 0;
                    m_pInstance->SetData(TYPE_NAGMARA, SPECIAL);
                }
                break;
            case 4:     // Phase 4 : make the lovers face each other
                m_creature->SetFacingToObject(pRocknot);
                pRocknot->SetFacingToObject(m_creature);
                m_uiPhaseTimer = 4000;
                m_uiPhase++;
                m_pInstance->SetData(TYPE_NAGMARA, DONE);
                break;
            case 5:     // Phase 5 : Nagmara and Rocknot are under the stair kissing (this phase repeats endlessly)
                DoScriptText(TEXTEMOTE_NAGMARA, m_creature);
                DoScriptText(TEXTEMOTE_ROCKNOT, pRocknot);
                DoCastSpellIfCan(m_creature, SPELL_NAGMARA_ROCKNOT);
                pRocknot->CastSpell(pRocknot, SPELL_NAGMARA_ROCKNOT, false);
                m_uiPhaseTimer = 12000;
                break;
        }
    }
};

struct npc_mistress_nagmara : public CreatureScript
{
    npc_mistress_nagmara() : CreatureScript("npc_mistress_nagmara") {}

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_mistress_nagmaraAI(pCreature);
    }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (pCreature->IsQuestGiver())
        {
            pPlayer->PrepareQuestMenu(pCreature->GetObjectGuid());
        }

        if (pPlayer->GetQuestStatus(QUEST_POTION_LOVE) == QUEST_STATUS_COMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_NAGMARA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->SEND_GOSSIP_MENU(GOSSIP_ID_NAGMARA_2, pCreature->GetObjectGuid());
        }
        else
        {
            pPlayer->SEND_GOSSIP_MENU(GOSSIP_ID_NAGMARA, pCreature->GetObjectGuid());
        }

        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        switch (uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF+1:
                pPlayer->CLOSE_GOSSIP_MENU();
                if (npc_mistress_nagmaraAI* pNagmaraAI = dynamic_cast<npc_mistress_nagmaraAI*>(pCreature->AI()))
                {
                    pNagmaraAI->DoPotionOfLoveIfCan();
                }
                break;
        }
        return true;
    }

    bool OnQuestRewarded(Player* /*pPlayer*/, Creature* pCreature, Quest const* pQuest) override
    {
        ScriptedInstance* pInstance = (ScriptedInstance*)pCreature->GetInstanceData();

        if (!pInstance)
        {
            return true;
        }

        if (pQuest->GetQuestId() == QUEST_POTION_LOVE)
        {
            if (npc_mistress_nagmaraAI* pNagmaraAI = dynamic_cast<npc_mistress_nagmaraAI*>(pCreature->AI()))
            {
                pNagmaraAI->DoPotionOfLoveIfCan();
            }
        }

        return true;
    }
};

/*######
## npc_rocknot
######*/

enum
{
    SAY_GOT_BEER       = -1230000,

    SPELL_DRUNKEN_RAGE = 14872,

    QUEST_ALE          = 4295
};

struct npc_rocknot : public CreatureScript
{
    npc_rocknot() : CreatureScript("npc_rocknot") {}

    struct npc_rocknotAI : public npc_escortAI
    {
        npc_rocknotAI(Creature* pCreature) : npc_escortAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
            Reset();
        }

        ScriptedInstance* m_pInstance;

        uint32 m_uiBreakKegTimer;
        uint32 m_uiBreakDoorTimer;

        void Reset() override
        {
            if (HasEscortState(STATE_ESCORT_ESCORTING))
            {
                return;
            }

            m_uiBreakKegTimer = 0;
            m_uiBreakDoorTimer = 0;
        }

        void DoGo(uint32 id, uint32 state)
        {
            if (GameObject* pGo = m_pInstance->GetSingleGameObjectFromStorage(id))
            {
                pGo->SetGoState(GOState(state));
            }
        }

        void WaypointReached(uint32 uiPointId) override
        {
            if (!m_pInstance)
            {
                return;
            }

            switch (uiPointId)
            {
            case 1:
                m_creature->HandleEmote(EMOTE_ONESHOT_KICK);
                break;
            case 2:
                m_creature->HandleEmote(EMOTE_ONESHOT_ATTACKUNARMED);
                break;
            case 3:
                m_creature->HandleEmote(EMOTE_ONESHOT_ATTACKUNARMED);
                break;
            case 4:
                m_creature->HandleEmote(EMOTE_ONESHOT_KICK);
                break;
            case 5:
                m_creature->HandleEmote(EMOTE_ONESHOT_KICK);
                m_uiBreakKegTimer = 2000;
                break;
            }
        }

        void UpdateEscortAI(const uint32 uiDiff) override
        {
            if (!m_pInstance)
            {
                return;
            }

            if (m_uiBreakKegTimer)
            {
                if (m_uiBreakKegTimer <= uiDiff)
                {
                    DoGo(GO_BAR_KEG_SHOT, 0);
                    m_uiBreakKegTimer = 0;
                    m_uiBreakDoorTimer = 1000;
                }
                else
                {
                    m_uiBreakKegTimer -= uiDiff;
                }
            }

            if (m_uiBreakDoorTimer)
            {
                if (m_uiBreakDoorTimer <= uiDiff)
                {
                    DoGo(GO_BAR_DOOR, 2);
                    DoGo(GO_BAR_KEG_TRAP, 0);                   // doesn't work very well, leaving code here for future
                    // spell by trap has effect61, this indicate the bar go hostile

                    if (Creature* pTmp = m_pInstance->GetSingleCreatureFromStorage(NPC_PHALANX))
                    {
                        pTmp->SetFactionTemporary(14, TEMPFACTION_NONE);
                    }

                    // for later, this event(s) has alot more to it.
                    // optionally, DONE can trigger bar to go hostile.
                    m_pInstance->SetData(TYPE_BAR, DONE);

                    m_uiBreakDoorTimer = 0;
                }
                else
                {
                    m_uiBreakDoorTimer -= uiDiff;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_rocknotAI(pCreature);
    }

    bool OnQuestRewarded(Player* /*pPlayer*/, Creature* pCreature, Quest const* pQuest) override
    {
        ScriptedInstance* pInstance = (ScriptedInstance*)pCreature->GetInstanceData();

        if (!pInstance)
        {
            return true;
        }

        if (pInstance->GetData(TYPE_BAR) == DONE || pInstance->GetData(TYPE_BAR) == SPECIAL)
        {
            return true;
        }

        if (pQuest->GetQuestId() == QUEST_ALE)
        {
            if (pInstance->GetData(TYPE_BAR) != IN_PROGRESS)
            {
                pInstance->SetData(TYPE_BAR, IN_PROGRESS);
            }

            pInstance->SetData(TYPE_BAR, SPECIAL);

            // keep track of amount in instance script, returns SPECIAL if amount ok and event in progress
            if (pInstance->GetData(TYPE_BAR) == SPECIAL)
            {
                DoScriptText(SAY_GOT_BEER, pCreature);
                pCreature->CastSpell(pCreature, SPELL_DRUNKEN_RAGE, false);

                if (npc_rocknotAI* pEscortAI = dynamic_cast<npc_rocknotAI*>(pCreature->AI()))
                {
                    pEscortAI->Start(false, nullptr, nullptr, true);
                }
            }
        }

        return true;
    }
};


/*######
## npc_marshal_windsor
######*/

enum
{
    // Windsor texts
    SAY_WINDSOR_AGGRO1          = -1230011,
    SAY_WINDSOR_AGGRO2          = -1230012,
    SAY_WINDSOR_AGGRO3          = -1230013,
    SAY_WINDSOR_START           = -1230014,
    SAY_WINDSOR_CELL_DUGHAL_1   = -1230015,
    SAY_WINDSOR_CELL_DUGHAL_3   = -1230016,
    SAY_WINDSOR_EQUIPMENT_1     = -1230017,
    SAY_WINDSOR_EQUIPMENT_2     = -1230018,
    SAY_WINDSOR_EQUIPMENT_3     = -1230019,
    SAY_WINDSOR_EQUIPMENT_4     = -1230020,
    SAY_WINDSOR_CELL_JAZ_1      = -1230021,
    SAY_WINDSOR_CELL_JAZ_2      = -1230022,
    SAY_WINDSOR_CELL_SHILL_1    = -1230023,
    SAY_WINDSOR_CELL_SHILL_2    = -1230024,
    SAY_WINDSOR_CELL_SHILL_3    = -1230025,
    SAY_WINDSOR_CELL_CREST_1    = -1230026,
    SAY_WINDSOR_CELL_CREST_2    = -1230027,
    SAY_WINDSOR_CELL_TOBIAS_1   = -1230028,
    SAY_WINDSOR_CELL_TOBIAS_2   = -1230029,
    SAY_WINDSOR_FREE_1          = -1230030,
    SAY_WINDSOR_FREE_2          = -1230031,

    // Additional gossips
    SAY_DUGHAL_FREE             = -1230010,
    GOSSIP_ID_DUGHAL            = -3230000,
    GOSSIP_TEXT_ID_DUGHAL       = 2846,

    SAY_TOBIAS_FREE_1           = -1230032,
    SAY_TOBIAS_FREE_2           = -1230033,
    GOSSIP_ID_TOBIAS            = -3230001,
    GOSSIP_TEXT_ID_TOBIAS       = 2847,

    NPC_REGINALD_WINDSOR        = 9682,

    QUEST_JAIL_BREAK            = 4322
};

struct npc_marshal_windsor : public CreatureScript
{
    npc_marshal_windsor() : CreatureScript("npc_marshal_windsor") {}

    struct npc_marshal_windsorAI : public npc_escortAI
    {
        npc_marshal_windsorAI(Creature* m_creature) : npc_escortAI(m_creature)
        {
            m_pInstance = (ScriptedInstance*)m_creature->GetInstanceData();
        }

        ScriptedInstance* m_pInstance;

        uint8 m_uiEventPhase;

        void Reset() override
        {
            if (!HasEscortState(STATE_ESCORT_ESCORTING))
            {
                m_uiEventPhase = 0;
            }
        }

        void Aggro(Unit* pWho) override
        {
            switch (urand(0, 2))
            {
            case 0:
                DoScriptText(SAY_WINDSOR_AGGRO1, m_creature, pWho);
                break;
            case 1:
                DoScriptText(SAY_WINDSOR_AGGRO2, m_creature);
                break;
            case 2:
                DoScriptText(SAY_WINDSOR_AGGRO3, m_creature, pWho);
                break;
            }
        }

        void WaypointReached(uint32 uiPointId) override
        {
            switch (uiPointId)
            {
            case 1:
                if (m_pInstance)
                {
                    m_pInstance->SetData(TYPE_QUEST_JAIL_BREAK, IN_PROGRESS);
                }

                DoScriptText(SAY_WINDSOR_START, m_creature);
                break;
            case 7:
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    DoScriptText(SAY_WINDSOR_CELL_DUGHAL_1, m_creature, pPlayer);
                }
                if (m_pInstance)
                {
                    if (Creature* pDughal = m_pInstance->GetSingleCreatureFromStorage(NPC_DUGHAL))
                    {
                        pDughal->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        m_creature->SetFacingToObject(pDughal);
                    }
                }
                ++m_uiEventPhase;
                SetEscortPaused(true);
                break;
            case 9:
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    DoScriptText(SAY_WINDSOR_CELL_DUGHAL_3, m_creature, pPlayer);
                }
                break;
            case 14:
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    DoScriptText(SAY_WINDSOR_EQUIPMENT_1, m_creature, pPlayer);
                }
                break;
            case 15:
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
                m_creature->HandleEmoteCommand(EMOTE_ONESHOT_USESTANDING);
#endif
                break;
            case 16:
                if (m_pInstance)
                {
                    m_pInstance->DoUseDoorOrButton(GO_JAIL_DOOR_SUPPLY);
                }
                break;
            case 18:
                DoScriptText(SAY_WINDSOR_EQUIPMENT_2, m_creature);
                break;
            case 19:
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
                m_creature->HandleEmoteCommand(EMOTE_ONESHOT_USESTANDING);
#endif
                break;
            case 20:
                if (m_pInstance)
                {
                    m_pInstance->DoUseDoorOrButton(GO_JAIL_SUPPLY_CRATE);
                }
                break;
            case 21:
                m_creature->UpdateEntry(NPC_REGINALD_WINDSOR);
                break;
            case 22:
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    DoScriptText(SAY_WINDSOR_EQUIPMENT_3, m_creature, pPlayer);
                    m_creature->SetFacingToObject(pPlayer);
                }
                break;
            case 23:
                DoScriptText(SAY_WINDSOR_EQUIPMENT_4, m_creature);
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    m_creature->SetFacingToObject(pPlayer);
                }
                break;
            case 30:
                if (m_pInstance)
                {
                    if (Creature* pJaz = m_pInstance->GetSingleCreatureFromStorage(NPC_JAZ))
                    {
                        m_creature->SetFacingToObject(pJaz);
                    }
                }
                DoScriptText(SAY_WINDSOR_CELL_JAZ_1, m_creature);
                ++m_uiEventPhase;
                SetEscortPaused(true);
                break;
            case 32:
                DoScriptText(SAY_WINDSOR_CELL_JAZ_2, m_creature);
                break;
            case 35:
                if (m_pInstance)
                {
                    if (Creature* pShill = m_pInstance->GetSingleCreatureFromStorage(NPC_SHILL))
                    {
                        m_creature->SetFacingToObject(pShill);
                    }
                }
                DoScriptText(SAY_WINDSOR_CELL_SHILL_1, m_creature);
                ++m_uiEventPhase;
                SetEscortPaused(true);
                break;
            case 37:
                DoScriptText(SAY_WINDSOR_CELL_SHILL_2, m_creature);
                break;
            case 38:
                DoScriptText(SAY_WINDSOR_CELL_SHILL_3, m_creature);
                break;
            case 45:
                if (m_pInstance)
                {
                    if (Creature* pCrest = m_pInstance->GetSingleCreatureFromStorage(NPC_CREST))
                    {
                        m_creature->SetFacingToObject(pCrest);
                    }
                }
                DoScriptText(SAY_WINDSOR_CELL_CREST_1, m_creature);
                ++m_uiEventPhase;
                SetEscortPaused(true);
                break;
            case 47:
                DoScriptText(SAY_WINDSOR_CELL_CREST_2, m_creature);
                break;
            case 49:
                DoScriptText(SAY_WINDSOR_CELL_TOBIAS_1, m_creature);
                if (m_pInstance)
                {
                    if (Creature* pTobias = m_pInstance->GetSingleCreatureFromStorage(NPC_TOBIAS))
                    {
                        pTobias->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        m_creature->SetFacingToObject(pTobias);
                    }
                }
                ++m_uiEventPhase;
                SetEscortPaused(true);
                break;
            case 51:
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    DoScriptText(SAY_WINDSOR_CELL_TOBIAS_2, m_creature, pPlayer);
                }
                break;
            case 57:
                DoScriptText(SAY_WINDSOR_FREE_1, m_creature);
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    m_creature->SetFacingToObject(pPlayer);
                }
                break;
            case 58:
                DoScriptText(SAY_WINDSOR_FREE_2, m_creature);
                if (m_pInstance)
                {
                    m_pInstance->SetData(TYPE_QUEST_JAIL_BREAK, DONE);
                }

                if (Player* pPlayer = GetPlayerForEscort())
                {
                    pPlayer->GroupEventHappens(QUEST_JAIL_BREAK, m_creature);
                }
                break;
            }
        }

        void UpdateEscortAI(const uint32 /*uiDiff*/) override
        {
            // Handle escort resume events
            if (m_pInstance && m_pInstance->GetData(TYPE_QUEST_JAIL_BREAK) == SPECIAL)
            {
                switch (m_uiEventPhase)
                {
                case 1:                     // Dughal
                case 3:                     // Ograbisi
                case 4:                     // Crest
                case 5:                     // Shill
                case 6:                     // Tobias
                    SetEscortPaused(false);
                    break;
                case 2:                     // Jaz
                    ++m_uiEventPhase;
                    break;
                }

                m_pInstance->SetData(TYPE_QUEST_JAIL_BREAK, IN_PROGRESS);
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_marshal_windsorAI(pCreature);
    }

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_JAIL_BREAK)
        {
            pCreature->SetFactionTemporary(FACTION_ESCORT_A_NEUTRAL_ACTIVE, TEMPFACTION_RESTORE_RESPAWN);

            if (npc_marshal_windsorAI* pEscortAI = dynamic_cast<npc_marshal_windsorAI*>(pCreature->AI()))
            {
                pEscortAI->Start(false, pPlayer, pQuest);
            }

            return true;
        }

        return false;
    }
};

/*######
## npc_dughal_stormwing
######*/

struct npc_dughal_stormwing : public CreatureScript
{
    npc_dughal_stormwing() : CreatureScript("npc_dughal_stormwing") {}

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (pPlayer->GetQuestStatus(QUEST_JAIL_BREAK) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ID_DUGHAL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }

        pPlayer->SEND_GOSSIP_MENU(GOSSIP_TEXT_ID_DUGHAL, pCreature->GetObjectGuid());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            // Set instance data in order to allow the quest to continue
            if (InstanceData* pInstance = pCreature->GetInstanceData())
            {
                pInstance->SetData(TYPE_QUEST_JAIL_BREAK, SPECIAL);
            }

            DoScriptText(SAY_DUGHAL_FREE, pCreature, pPlayer);
            pCreature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

            pCreature->SetWalk(false);
            pCreature->GetMotionMaster()->MoveWaypoint();

            pPlayer->CLOSE_GOSSIP_MENU();
        }

        return true;
    }
};

/*######
## npc_tobias_seecher
######*/

struct npc_tobias_seecher : public CreatureScript
{
    npc_tobias_seecher() : CreatureScript("npc_tobias_seecher") {}

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (pPlayer->GetQuestStatus(QUEST_JAIL_BREAK) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ID_TOBIAS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }

        pPlayer->SEND_GOSSIP_MENU(GOSSIP_TEXT_ID_TOBIAS, pCreature->GetObjectGuid());

        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            // Set instance data in order to allow the quest to continue
            if (InstanceData* pInstance = pCreature->GetInstanceData())
            {
                pInstance->SetData(TYPE_QUEST_JAIL_BREAK, SPECIAL);
            }

            DoScriptText(urand(0, 1) ? SAY_TOBIAS_FREE_1 : SAY_TOBIAS_FREE_2, pCreature);
            pCreature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

            pCreature->SetWalk(false);
            pCreature->GetMotionMaster()->MoveWaypoint();

            pPlayer->CLOSE_GOSSIP_MENU();
        }

        return true;
    }
};


/*######
 ## npc_hurley_blackbreath
 ######*/

enum
{
    YELL_HURLEY_SPAWN      = -1230041,
    SAY_HURLEY_AGGRO       = -1230042,

    // SPELL_DRUNKEN_RAGE      = 14872,
    SPELL_FLAME_BREATH     = 9573,

    NPC_RIBBLY_SCREWSPIGOT = 9543,
    NPC_RIBBLY_CRONY       = 10043,
};

struct npc_hurley_blackbreath : public CreatureScript
{
    npc_hurley_blackbreath() : CreatureScript("npc_hurley_blackbreath") {}

    struct npc_hurley_blackbreathAI : public npc_escortAI
    {
        npc_hurley_blackbreathAI(Creature* pCreature) : npc_escortAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();

            Reset();
        }

        ScriptedInstance* m_pInstance;

        uint32 uiFlameBreathTimer;
        uint32 m_uiEventTimer;
        bool   bIsEnraged;

        void Reset() override
        {
            // If reset after an fight, we made him move to the keg room (end of the path)
            if (HasEscortState(STATE_ESCORT_ESCORTING) || HasEscortState(STATE_ESCORT_PAUSED))
            {
                SetCurrentWaypoint(5);
                SetEscortPaused(false);
            }
            else
            {
                m_uiEventTimer  = 1000;
            }

            bIsEnraged          = false;
        }

        // We want to prevent Hurley to go rampage on Ribbly and his friends.
        // Everybody loves Ribbly. Except his family. They want him dead.
        void AttackStart(Unit* pWho) override
        {
            if (pWho)
            {
                if (pWho->GetEntry() == NPC_RIBBLY_SCREWSPIGOT || pWho->GetEntry() == NPC_RIBBLY_CRONY)
                {
                    return;
                }
                else
                {
                    ScriptedAI::AttackStart(pWho);
                }
            }
        }

        void Aggro(Unit* /*pWho*/) override
        {
            uiFlameBreathTimer  = 7000;
            bIsEnraged  = false;
            DoScriptText(SAY_HURLEY_AGGRO, m_creature);
        }

        void WaypointReached(uint32 uiPointId) override
        {
            if (!m_pInstance)
            {
                return;
            }

            switch (uiPointId)
            {
                case 1:
                    DoScriptText(YELL_HURLEY_SPAWN, m_creature);
                    SetRun(true);
                    break;
                case 5:
                    SetEscortPaused(true);
                    break;
                default:
                    break;
            }
        }

        void UpdateEscortAI(const uint32 uiDiff) override
        {
            if (!m_pInstance)
            {
                return;
            }

            // Combat check
            if (m_creature->SelectHostileTarget() && m_creature->getVictim())
            {
                if (uiFlameBreathTimer < uiDiff)
                {
                    if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FLAME_BREATH) == CAST_OK)
                    {
                        uiFlameBreathTimer = 10000;
                    }
                }
                else
                {
                    uiFlameBreathTimer -= uiDiff;
                }

                if (m_creature->GetHealthPercent() < 31.0f && !bIsEnraged)
                {
                    if (DoCastSpellIfCan(m_creature, SPELL_DRUNKEN_RAGE) == CAST_OK)
                    {
                        bIsEnraged = true;
                    }
                }

                DoMeleeAttackIfReady();
            }
            else
            {
                if (m_uiEventTimer)
                {
                    if (m_uiEventTimer < uiDiff)
                    {
                        Start(false);
                        SetEscortPaused(false);
                        m_uiEventTimer = 0;
                    }
                    else
                    {
                        m_uiEventTimer -= uiDiff;
                    }
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_hurley_blackbreathAI(pCreature);
    }
};


/*######
## boss_doomrel
######*/

enum
{
    SAY_DOOMREL_START_EVENT     = -1230003,
    GOSSIP_ITEM_CHALLENGE       = -3230002,
    GOSSIP_TEXT_ID_CHALLENGE    = 2601,
};

struct boss_doomrel : public CreatureScript
{
    boss_doomrel() : CreatureScript("boss_doomrel") {}

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (ScriptedInstance* pInstance = (ScriptedInstance*)pCreature->GetInstanceData())
        {
            if (pInstance->GetData(TYPE_TOMB_OF_SEVEN) == NOT_STARTED || pInstance->GetData(TYPE_TOMB_OF_SEVEN) == FAIL)
            {
                pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_CHALLENGE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            }
        }

        pPlayer->SEND_GOSSIP_MENU(GOSSIP_TEXT_ID_CHALLENGE, pCreature->GetObjectGuid());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        switch (uiAction)
        {
        case GOSSIP_ACTION_INFO_DEF + 1:
            pPlayer->CLOSE_GOSSIP_MENU();
            DoScriptText(SAY_DOOMREL_START_EVENT, pCreature);
            // start event
            if (ScriptedInstance* pInstance = (ScriptedInstance*)pCreature->GetInstanceData())
            {
                pInstance->SetData(TYPE_TOMB_OF_SEVEN, IN_PROGRESS);
            }

            break;
        }
        return true;
    }
};


/*######
## boss_plugger_spazzring
######*/

enum
{
    SAY_OOC_1                       = -1230050,
    SAY_OOC_2                       = -1230051,
    SAY_OOC_3                       = -1230052,
    SAY_OOC_4                       = -1230053,

    YELL_STOLEN_1                   = -1230054,
    YELL_STOLEN_2                   = -1230055,
    YELL_STOLEN_3                   = -1230056,
    YELL_AGRRO_1                    = -1230057,
    YELL_AGRRO_2                    = -1230058,
    YELL_PICKPOCKETED               = -1230059,

    // spells
    SPELL_BANISH                    = 8994,
    SPELL_CURSE_OF_TONGUES          = 13338,
    SPELL_DEMON_ARMOR               = 13787,
    SPELL_IMMOLATE                  = 12742,
    SPELL_SHADOW_BOLT               = 12739,
    SPELL_PICKPOCKET                = 921,
};

static const int aRandomSays[] = { SAY_OOC_1, SAY_OOC_2, SAY_OOC_3, SAY_OOC_4 };

static const int aRandomYells[] = { YELL_STOLEN_1, YELL_STOLEN_2, YELL_STOLEN_3 };

struct boss_plugger_spazzringAI : public ScriptedAI
{
    boss_plugger_spazzringAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (instance_blackrock_depths*)pCreature->GetInstanceData();
        Reset();
    }

    instance_blackrock_depths* m_pInstance;

    uint32 m_uiOocSayTimer;
    uint32 m_uiDemonArmorTimer;
    uint32 m_uiBanishTimer;
    uint32 m_uiImmolateTimer;
    uint32 m_uiShadowBoltTimer;
    uint32 m_uiCurseOfTonguesTimer;
    uint32 m_uiPickpocketTimer;

    void Reset() override
    {
        m_uiOocSayTimer          = 10000;
        m_uiDemonArmorTimer      = 1000;
        m_uiBanishTimer          = 0;
        m_uiImmolateTimer        = 0;
        m_uiShadowBoltTimer      = 0;
        m_uiCurseOfTonguesTimer  = 0;
        m_uiPickpocketTimer      = 0;
    }

    void Aggro(Unit* /*pWho*/) override
    {
        m_uiBanishTimer          = urand(8, 12) * 1000;
        m_uiImmolateTimer        = urand(18, 20) * 1000;
        m_uiShadowBoltTimer      = 1000;
        m_uiCurseOfTonguesTimer  = 17000;
    }

    void JustDied(Unit* pKiller) override
    {
        if (!m_pInstance)
        {
            return;
        }

        // Activate Phalanx and handle patrons faction
        if (Creature* pPhalanx = m_pInstance->GetSingleCreatureFromStorage(NPC_PHALANX))
        {
            if (!m_pInstance)
            {
                return;
            }

            // Activate Phalanx and handle patrons faction
            if (Creature* pPhalanx = m_pInstance->GetSingleCreatureFromStorage(NPC_PHALANX))
            {
                if (npc_phalanxAI* pEscortAI = dynamic_cast<npc_phalanxAI*>(pPhalanx->AI()))
                {
                    pEscortAI->Start(false, nullptr, nullptr, true);
                }
            }
            m_pInstance->HandleBarPatrons(PATRON_HOSTILE);
            m_pInstance->SetData(TYPE_PLUGGER, IN_PROGRESS); // The event is set IN_PROGRESS even if Plugger is dead because his death triggers more actions that are part of the event
        }
    }

    void SpellHit(Unit* pCaster, const SpellEntry* pSpell) override
    {
        if (pCaster->GetTypeId() == TYPEID_PLAYER)
        {
            if (pSpell->Id == SPELL_PICKPOCKET)
            {
                m_uiPickpocketTimer = 5000;
            }
        }
    }

    // Players stole one of the ale mug/roasted boar: warn them
    void WarnThief(Player* pPlayer)
    {
        DoScriptText(aRandomYells[urand(0, 2)], m_creature);
        m_creature->SetFacingToObject(pPlayer);
    }

    // Players stole too much of the ale mug/roasted boar: attack them
    void AttackThief(Player* pPlayer)
    {
        if (pPlayer)
        {
            DoScriptText(urand(0, 1) < 1 ? YELL_AGRRO_1 : YELL_AGRRO_2, m_creature);
            m_creature->SetFacingToObject(pPlayer);
            m_creature->SetFactionTemporary(FACTION_DARK_IRON, TEMPFACTION_RESTORE_RESPAWN);
            AttackStart(pPlayer);
        }
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        // Combat check
        if (m_creature->SelectHostileTarget() && m_creature->getVictim())
        {
            if (m_uiBanishTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_BANISH) == CAST_OK)
                    {
                        m_uiBanishTimer = urand(26, 28) * 1000;
                    }
                }
            }
            else
            {
                m_uiBanishTimer -= uiDiff;
            }

            if (m_uiImmolateTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_IMMOLATE) == CAST_OK)
                {
                    m_uiImmolateTimer = 25000;
                }
            }
            else
            {
                m_uiImmolateTimer -= uiDiff;
            }

            if (m_uiShadowBoltTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_SHADOW_BOLT) == CAST_OK)
                {
                    m_uiShadowBoltTimer = urand(36, 63) * 100;
                }
            }
            else
            {
                m_uiShadowBoltTimer -= uiDiff;
            }

            if (m_uiCurseOfTonguesTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_CURSE_OF_TONGUES, SELECT_FLAG_POWER_MANA))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_CURSE_OF_TONGUES) == CAST_OK)
                    {
                        m_uiCurseOfTonguesTimer = urand(19, 31) * 1000;
                    }
                }
            }
            else
            {
                m_uiCurseOfTonguesTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
        // Out of Combat (OOC)
        else
        {
            if (m_uiOocSayTimer < uiDiff)
            {
                DoScriptText(aRandomSays[urand(0, 3)], m_creature);
                m_uiOocSayTimer = urand(10, 20) * 1000;
            }
            else
            {
                m_uiOocSayTimer -= uiDiff;
            }

            if (m_uiPickpocketTimer)
            {
                if (m_uiPickpocketTimer < uiDiff)
                {
                    DoScriptText(YELL_PICKPOCKETED, m_creature);
                    m_creature->SetFactionTemporary(FACTION_DARK_IRON, TEMPFACTION_RESTORE_RESPAWN);
                    m_uiPickpocketTimer = 0;
                }
                else
                {
                    m_uiPickpocketTimer -= uiDiff;
                }
            }

            if (m_uiDemonArmorTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_DEMON_ARMOR) == CAST_OK)
                {
                    m_uiDemonArmorTimer = 30 * MINUTE * IN_MILLISECONDS;
                }
            }
            else
            {
                m_uiDemonArmorTimer -= uiDiff;
            }
        }
    }
};

struct boss_plugger_spazzring : public CreatureScript
{
    boss_plugger_spazzring() : CreatureScript("boss_plugger_spazzring") {}


    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_plugger_spazzringAI(pCreature);
    }
};


/*######
## npc_kharan_mighthammer
######*/
enum
{
    QUEST_WHAT_IS_GOING_ON = 4001,
    QUEST_KHARANS_TALE     = 4342
};

#define GOSSIP_ITEM_KHARAN_1    "I need to know where the princess are, Kharan!"
#define GOSSIP_ITEM_KHARAN_2    "All is not lost, Kharan!"

#define GOSSIP_ITEM_KHARAN_3    "Gor'shak is my friend, you can trust me."
#define GOSSIP_ITEM_KHARAN_4    "Not enough, you need to tell me more."
#define GOSSIP_ITEM_KHARAN_5    "So what happened?"
#define GOSSIP_ITEM_KHARAN_6    "Continue..."
#define GOSSIP_ITEM_KHARAN_7    "So you suspect that someone on the inside was involved? That they were tipped off?"
#define GOSSIP_ITEM_KHARAN_8    "Continue with your story please."
#define GOSSIP_ITEM_KHARAN_9    "Indeed."
#define GOSSIP_ITEM_KHARAN_10   "The door is open, Kharan. You are a free man."

struct npc_kharan_mighthammer : public CreatureScript
{
    npc_kharan_mighthammer() : CreatureScript("npc_kharan_mighthammer") {}

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (pCreature->IsQuestGiver())
        {
            pPlayer->PrepareQuestMenu(pCreature->GetObjectGuid());
        }

        if (pPlayer->GetQuestStatus(QUEST_WHAT_IS_GOING_ON) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }

        if (pPlayer->GetQuestStatus(QUEST_KHARANS_TALE) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        }

        if (pPlayer->GetTeam() == HORDE)
        {
            pPlayer->SEND_GOSSIP_MENU(2473, pCreature->GetObjectGuid());
        }
        else
        {
            pPlayer->SEND_GOSSIP_MENU(2474, pCreature->GetObjectGuid());
        }

        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        switch (uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF+1:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                pPlayer->SEND_GOSSIP_MENU(2475, pCreature->GetObjectGuid());
                break;
            case GOSSIP_ACTION_INFO_DEF+2:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
                pPlayer->SEND_GOSSIP_MENU(2476, pCreature->GetObjectGuid());
                break;

            case GOSSIP_ACTION_INFO_DEF+3:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_5, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
                pPlayer->SEND_GOSSIP_MENU(2477, pCreature->GetObjectGuid());
                break;
            case GOSSIP_ACTION_INFO_DEF+4:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_6, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
                pPlayer->SEND_GOSSIP_MENU(2478, pCreature->GetObjectGuid());
                break;
            case GOSSIP_ACTION_INFO_DEF+5:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_7, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);
                pPlayer->SEND_GOSSIP_MENU(2479, pCreature->GetObjectGuid());
                break;
            case GOSSIP_ACTION_INFO_DEF+6:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_8, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);
                pPlayer->SEND_GOSSIP_MENU(2480, pCreature->GetObjectGuid());
                break;
            case GOSSIP_ACTION_INFO_DEF+7:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_9, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 8);
                pPlayer->SEND_GOSSIP_MENU(2481, pCreature->GetObjectGuid());
                break;
            case GOSSIP_ACTION_INFO_DEF+8:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_10, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 9);
                pPlayer->SEND_GOSSIP_MENU(2482, pCreature->GetObjectGuid());
                break;
            case GOSSIP_ACTION_INFO_DEF+9:
                pPlayer->CLOSE_GOSSIP_MENU();
                if (pPlayer->GetTeam() == HORDE)
                {
                    pPlayer->AreaExploredOrEventHappens(QUEST_WHAT_IS_GOING_ON);
                }
                else
                {
                    pPlayer->AreaExploredOrEventHappens(QUEST_KHARANS_TALE);
                }
                break;
        }
        return true;
    }
};

/*######
## go_bar_ale_mug
######*/
struct go_bar_ale_mug : public GameObjectScript
{
    go_bar_ale_mug() : GameObjectScript("go_bar_ale_mug") {}

    bool OnUse(Player* pPlayer, GameObject* pGo) override
    {
        if (ScriptedInstance* pInstance = (ScriptedInstance*)pGo->GetInstanceData())
        {
            if (pInstance->GetData(TYPE_PLUGGER) == IN_PROGRESS || pInstance->GetData(TYPE_PLUGGER) == DONE)
            {
                return false;
            }
            else
            {
                if (Creature* pPlugger = pInstance->GetSingleCreatureFromStorage(NPC_PLUGGER_SPAZZRING))
                {
                    if (boss_plugger_spazzringAI* pPluggerAI = dynamic_cast<boss_plugger_spazzringAI*>(pPlugger->AI()))
                    {
                        pInstance->SetData(TYPE_PLUGGER, SPECIAL);
                        if (pInstance->GetData(TYPE_PLUGGER) == IN_PROGRESS)
                        {
                            pPluggerAI->AttackThief(pPlayer);
                        }
                        else
                        {
                            pPluggerAI->WarnThief(pPlayer);
                        }
                    }
                }
            }
        }
        return false;
    }
};

/*######
## npc_ironhand_guardian
######*/

enum
{
    SPELL_GOUT_OF_FLAME     = 15529,
    SPELL_STONED_VISUAL     = 15533,
};

struct npc_ironhand_guardian : public CreatureScript
{
    npc_ironhand_guardian() : CreatureScript("npc_ironhand_guardian") {}

    struct npc_ironhand_guardianAI : public ScriptedAI
    {
        npc_ironhand_guardianAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
            Reset();
        }

        ScriptedInstance* m_pInstance;

        uint32 m_uiGoutOfFlameTimer;
        uint8 m_uiPhase;

        void Reset() override
        {
            m_uiGoutOfFlameTimer    = urand(4, 8) * 1000;
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_pInstance)
            {
                return;
            }

            if (m_pInstance->GetData(TYPE_IRON_HALL) == NOT_STARTED)
            {
                m_uiPhase = 0;
                return;
            }

            switch (m_uiPhase)
            {
                case 0:
                    m_creature->RemoveAurasDueToSpell(SPELL_STONED);
                    if (DoCastSpellIfCan(m_creature, SPELL_STONED_VISUAL) == CAST_OK)
                    {
                        m_uiPhase = 1;
                    }
                    break;
                case 1:
                        if (m_uiGoutOfFlameTimer < uiDiff)
                        {
                            if (DoCastSpellIfCan(m_creature, SPELL_GOUT_OF_FLAME) == CAST_OK)
                            {
                                m_uiGoutOfFlameTimer = urand(13, 18) * 1000;
                            }
                        }
                        else
                        {
                            m_uiGoutOfFlameTimer -= uiDiff;
                        }
                    break;
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_ironhand_guardianAI(pCreature);
    }
};

void AddSC_blackrock_depths()
{
    Script *s;
    s = new go_bar_beer_keg();
    s->RegisterSelf();
    s = new go_shadowforge_brazier();
    s->RegisterSelf();
    s = new go_relic_coffer_door();
    s->RegisterSelf();
    s = new at_shadowforge_bridge();
    s->RegisterSelf();
    s = new at_ring_of_law();
    s->RegisterSelf();
    s = new npc_grimstone();
    s->RegisterSelf();
    s = new npc_phalanx();
    s->RegisterSelf();
    s = new npc_mistress_nagmara();
    s->RegisterSelf();
    s = new npc_rocknot();
    s->RegisterSelf();
    s = new npc_marshal_windsor();
    s->RegisterSelf();
    s = new npc_dughal_stormwing();
    s->RegisterSelf();
    s = new npc_tobias_seecher();
    s->RegisterSelf();
    s = new npc_hurley_blackbreath();
    s->RegisterSelf();
    s = new boss_doomrel();
    s->RegisterSelf();
    s = new boss_plugger_spazzring();
    s->RegisterSelf();
    s = new npc_kharan_mighthammer();
    s->RegisterSelf();
    s = new go_bar_ale_mug();
    s->RegisterSelf();
    s = new npc_ironhand_guardian();
    s->RegisterSelf();
    s = new spell_banner_of_provocation();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "go_shadowforge_brazier";
    //pNewScript->pGOUse = &GOUse_go_shadowforge_brazier;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "go_relic_coffer_door";
    //pNewScript->pGOUse = &GOUse_go_relic_coffer_door;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "at_ring_of_law";
    //pNewScript->pAreaTrigger = &AreaTrigger_at_ring_of_law;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_grimstone";
    //pNewScript->GetAI = &GetAI_npc_grimstone;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_theldren_trigger";
    //pNewScript->pEffectDummyNPC = &EffectDummyCreature_spell_banner_of_provocation;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_rocknot";
    //pNewScript->GetAI = &GetAI_npc_rocknot;
    //pNewScript->pQuestRewardedNPC = &QuestRewarded_npc_rocknot;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_marshal_windsor";
    //pNewScript->GetAI = &GetAI_npc_marshal_windsor;
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_marshal_windsor;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_tobias_seecher";
    //pNewScript->pGossipHello =  &GossipHello_npc_tobias_seecher;
    //pNewScript->pGossipSelect = &GossipSelect_npc_tobias_seecher;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_dughal_stormwing";
    //pNewScript->pGossipHello =  &GossipHello_npc_dughal_stormwing;
    //pNewScript->pGossipSelect = &GossipSelect_npc_dughal_stormwing;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_doomrel";
    //pNewScript->pGossipHello = &GossipHello_boss_doomrel;
    //pNewScript->pGossipSelect = &GossipSelect_boss_doomrel;
    //pNewScript->RegisterSelf();
}
