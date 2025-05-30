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
 * SDName:      boss_cannon_master_willey
 * SD%Complete: 100
 * SDComment:   None
 * SDCategory:  Stratholme
 * EndScriptData
 */

enum
{
    SPELL_KNOCK_AWAY        = 10101,
    SPELL_PUMMEL            = 15615,
    SPELL_SHOOT             = 16496,
    SPELL_SUMMON_RIFLEMAN   = 17279,        // spell needs script target
};

struct boss_cannon_master_willey : public CreatureScript
{
    boss_cannon_master_willey() : CreatureScript("boss_cannon_master_willey") {}

    struct boss_cannon_master_willeyAI : public ScriptedAI
    {
        boss_cannon_master_willeyAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint32 m_uiKnockAwayTimer;
        uint32 m_uiPummelTimer;
        uint32 m_uiShootTimer;
        uint32 m_uiSummonRiflemanTimer;

        void Reset() override
        {
            m_uiShootTimer = 1000;
            m_uiPummelTimer = 7000;
            m_uiKnockAwayTimer = 11000;
            m_uiSummonRiflemanTimer = 15000;
        }

        void JustSummoned(Creature* pSummoned) override
        {
            if (m_creature->getVictim())
            {
                pSummoned->AI()->AttackStart(m_creature->getVictim());
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // Return since we have no target
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Pummel
            if (m_uiPummelTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_PUMMEL) == CAST_OK)
                {
                    m_uiPummelTimer = 12000;
                }
            }
            else
            {
                m_uiPummelTimer -= uiDiff;
            }

            // KnockAway
            if (m_uiKnockAwayTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_KNOCK_AWAY) == CAST_OK)
                {
                    m_uiKnockAwayTimer = 14000;
                }
            }
            else
            {
                m_uiKnockAwayTimer -= uiDiff;
            }

            // Shoot
            if (m_uiShootTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_SHOOT, SELECT_FLAG_NOT_IN_MELEE_RANGE))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_SHOOT) == CAST_OK)
                    {
                        m_uiShootTimer = urand(3000, 4000);
                    }
                }
            }
            else
            {
                m_uiShootTimer -= uiDiff;
            }

            // SummonRifleman
            if (m_uiSummonRiflemanTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_SUMMON_RIFLEMAN) == CAST_OK)
                {
                    m_uiSummonRiflemanTimer = 30000;
                }
            }
            else
            {
                m_uiSummonRiflemanTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_cannon_master_willeyAI(pCreature);
    }
};

void AddSC_boss_cannon_master_willey()
{
    Script *s;
    s = new boss_cannon_master_willey();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_cannon_master_willey";
    //pNewScript->GetAI = &GetAI_boss_cannon_master_willey;
    //pNewScript->RegisterSelf();
}
