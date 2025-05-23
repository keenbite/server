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
 * SDName:      Boss_Flamegor
 * SD%Complete: 100
 * SDComment:   None
 * SDCategory:  Blackwing Lair
 * EndScriptData
 */

#include "blackwing_lair.h"

enum
{
    EMOTE_GENERIC_FRENZY        = -1000002,

    SPELL_SHADOW_FLAME          = 22539,
    SPELL_WING_BUFFET           = 23339,
    SPELL_FRENZY                = 23342,                    // This spell periodically triggers fire nova
    SPELL_THRASH                = 3391
};

struct boss_flamegor : public CreatureScript
{
    boss_flamegor() : CreatureScript("boss_flamegor") {}

    struct boss_flamegorAI : public ScriptedAI
    {
        boss_flamegorAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        }

        ScriptedInstance* m_pInstance;

        uint32 m_uiShadowFlameTimer;
        uint32 m_uiWingBuffetTimer;
        uint32 m_uiFrenzyTimer;
        uint32 m_uiTrashTimer;

        void Reset() override
        {
            m_uiShadowFlameTimer = 21000;                       // These times are probably wrong
            m_uiWingBuffetTimer = 35000;
            m_uiFrenzyTimer = 10000;
            m_uiTrashTimer = 25000;
        }

        void Aggro(Unit* /*pWho*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_FLAMEGOR, IN_PROGRESS);
            }
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_FLAMEGOR, DONE);
            }
        }

        void JustReachedHome() override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_FLAMEGOR, FAIL);
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Shadow Flame Timer
            if (m_uiShadowFlameTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_SHADOW_FLAME) == CAST_OK)
                {
                    m_uiShadowFlameTimer = urand(15000, 22000);
                }
            }
            else
            {
                m_uiShadowFlameTimer -= uiDiff;
            }

            // Wing Buffet Timer
            if (m_uiWingBuffetTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_WING_BUFFET) == CAST_OK)
                {
                    if (m_creature->GetThreatManager().getThreat(m_creature->getVictim()))
                    {
                        m_creature->GetThreatManager().modifyThreatPercent(m_creature->getVictim(), -75);
                    }

                    m_uiWingBuffetTimer = 25000;
                }
            }
            else
            {
                m_uiWingBuffetTimer -= uiDiff;
            }

            // Frenzy Timer
            if (m_uiFrenzyTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_FRENZY) == CAST_OK)
                {
                    DoScriptText(EMOTE_GENERIC_FRENZY, m_creature);
                    m_uiFrenzyTimer = urand(8000, 10000);
                }
            }
            else
            {
                m_uiFrenzyTimer -= uiDiff;
            }

            // Thrash Timer
            if (m_uiTrashTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_THRASH) == CAST_OK)
                {
                    m_uiTrashTimer = 20000;
                }
            }
            else
            {
                m_uiTrashTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_flamegorAI(pCreature);
    }
};

void AddSC_boss_flamegor()
{
    Script* s;
    s = new boss_flamegor();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_flamegor";
    //pNewScript->GetAI = &GetAI_boss_flamegor;
    //pNewScript->RegisterSelf();
}
