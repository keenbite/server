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
 * SDName:      Boss_Anubrekhan
 * SD%Complete: 95
 * SDComment:   Intro text usage is not very clear. Requires additional research.
 * SDCategory:  Naxxramas
 * EndScriptData
 */

#include "naxxramas.h"

enum
{
    SAY_GREET                   = -1533000,
    SAY_AGGRO1                  = -1533001,
    SAY_AGGRO2                  = -1533002,
    SAY_AGGRO3                  = -1533003,
    SAY_TAUNT1                  = -1533004,
    SAY_TAUNT2                  = -1533005,
    SAY_TAUNT3                  = -1533006,
    SAY_TAUNT4                  = -1533007,
    SAY_SLAY                    = -1533008,

    EMOTE_CRYPT_GUARD           = -1533153,
    EMOTE_INSECT_SWARM          = -1533154,
    EMOTE_CORPSE_SCARABS        = -1533155,

    SPELL_IMPALE                = 28783,                    // May be wrong spell id. Causes more dmg than I expect
    SPELL_LOCUSTSWARM           = 28785,                    // This is a self buff that triggers the dmg debuff

    // spellId invalid
    // SPELL_SUMMONGUARD         = 29508,                   // Summons 1 crypt guard at targeted location - spell doesn't exist in 3.x.x

    SPELL_SELF_SPAWN_5          = 29105,                    // This spawns 5 corpse scarabs ontop of us (most likely the pPlayer casts this on death)
    SPELL_SELF_SPAWN_10         = 28864,                    // This is used by the crypt guards when they die

    NPC_CRYPT_GUARD             = 16573
};

static const DialogueEntry aIntroDialogue[] =
{
    {SAY_GREET,   NPC_ANUB_REKHAN,  7000},
    {SAY_TAUNT1,  NPC_ANUB_REKHAN,  13000},
    {SAY_TAUNT2,  NPC_ANUB_REKHAN,  11000},
    {SAY_TAUNT3,  NPC_ANUB_REKHAN,  10000},
    {SAY_TAUNT4,  NPC_ANUB_REKHAN,  0},
    {0, 0, 0}
};

static const float aCryptGuardLoc[4] = {3333.5f, -3475.9f, 287.1f, 3.17f};

struct boss_anubrekhan : public CreatureScript
{
    boss_anubrekhan() : CreatureScript("boss_anubrekhan") {}

    struct boss_anubrekhanAI : public ScriptedAI
    {
        boss_anubrekhanAI(Creature* pCreature) : ScriptedAI(pCreature),
        m_introDialogue(aIntroDialogue)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
            m_introDialogue.InitializeDialogueHelper(m_pInstance);
            m_bHasTaunted = false;
        }

        ScriptedInstance* m_pInstance;
        DialogueHelper m_introDialogue;

        uint32 m_uiImpaleTimer;
        uint32 m_uiLocustSwarmTimer;
        uint32 m_uiSummonTimer;
        bool   m_bHasTaunted;

        void Reset() override
        {
            m_uiImpaleTimer = 15000;
            m_uiLocustSwarmTimer = 90000;
            m_uiSummonTimer = 0;
        }

        void KilledUnit(Unit* pVictim) override
        {
            // Force the player to spawn corpse scarabs via spell
            if (pVictim->GetTypeId() == TYPEID_PLAYER)
            {
                pVictim->CastSpell(pVictim, SPELL_SELF_SPAWN_5, true, nullptr, nullptr, m_creature->GetObjectGuid());
            }

            if (urand(0, 4))
            {
                return;
            }

            DoScriptText(SAY_SLAY, m_creature);
        }

        void Aggro(Unit* /*pWho*/) override
        {
            switch (urand(0, 2))
            {
            case 0:
                DoScriptText(SAY_AGGRO1, m_creature);
                break;
            case 1:
                DoScriptText(SAY_AGGRO2, m_creature);
                break;
            case 2:
                DoScriptText(SAY_AGGRO3, m_creature);
                break;
            }

            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_ANUB_REKHAN, IN_PROGRESS);
            }
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_ANUB_REKHAN, DONE);
            }
        }

        void JustReachedHome() override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_ANUB_REKHAN, FAIL);
            }
        }

        void MoveInLineOfSight(Unit* pWho) override
        {
            if (!m_bHasTaunted && pWho->GetTypeId() == TYPEID_PLAYER && m_creature->IsWithinDistInMap(pWho, 110.0f) && m_creature->IsWithinLOSInMap(pWho))
            {
                m_introDialogue.StartNextDialogueText(SAY_GREET);
                m_bHasTaunted = true;
            }

            ScriptedAI::MoveInLineOfSight(pWho);
        }

        void JustSummoned(Creature* pSummoned) override
        {
            if (pSummoned->GetEntry() == NPC_CRYPT_GUARD)
            {
                DoScriptText(EMOTE_CRYPT_GUARD, pSummoned);
            }

            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                pSummoned->AI()->AttackStart(pTarget);
            }
        }

        void SummonedCreatureDespawn(Creature* pSummoned) override
        {
            // If creature despawns on out of combat, skip this
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            if (pSummoned->GetEntry() == NPC_CRYPT_GUARD)
            {
                pSummoned->CastSpell(pSummoned, SPELL_SELF_SPAWN_10, true, nullptr, nullptr, m_creature->GetObjectGuid());
                DoScriptText(EMOTE_CORPSE_SCARABS, pSummoned);
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            m_introDialogue.DialogueUpdate(uiDiff);

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Impale
            if (m_uiImpaleTimer < uiDiff)
            {
                // Cast Impale on a random target
                // Do NOT cast it when we are afflicted by locust swarm
                if (!m_creature->HasAura(SPELL_LOCUSTSWARM))
                {
                    if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                    {
                        DoCastSpellIfCan(pTarget, SPELL_IMPALE);
                    }
                }

                m_uiImpaleTimer = 15000;
            }
            else
            {
                m_uiImpaleTimer -= uiDiff;
            }

            // Locust Swarm
            if (m_uiLocustSwarmTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_LOCUSTSWARM) == CAST_OK)
                {
                    DoScriptText(EMOTE_INSECT_SWARM, m_creature);

                    // Summon a crypt guard
                    m_uiSummonTimer = 3000;
                    m_uiLocustSwarmTimer = 100000;
                }
            }
            else
            {
                m_uiLocustSwarmTimer -= uiDiff;
            }

            // Summon
            if (m_uiSummonTimer)
            {
                if (m_uiSummonTimer <= uiDiff)
                {
                    // Workaround for the not existing spell
                    m_creature->SummonCreature(NPC_CRYPT_GUARD, aCryptGuardLoc[0], aCryptGuardLoc[1], aCryptGuardLoc[2], aCryptGuardLoc[3], TEMPSPAWN_TIMED_OOC_OR_DEAD_DESPAWN, 30000);
                    m_uiSummonTimer = 0;
                }
                else
                {
                    m_uiSummonTimer -= uiDiff;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_anubrekhanAI(pCreature);
    }
};

void AddSC_boss_anubrekhan()
{
    Script* s;
    s = new boss_anubrekhan();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_anubrekhan";
    //pNewScript->GetAI = &GetAI_boss_anubrekhan;
    //pNewScript->RegisterSelf();
}
