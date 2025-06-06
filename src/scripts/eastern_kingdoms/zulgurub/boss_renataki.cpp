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
 * SDName:      Boss_Renataki
 * SD%Complete: 100
 * SDComment:   None
 * SDCategory:  Zul'Gurub
 * EndScriptData
 */

enum
{
    SPELL_THOUSAND_BLADES   = 24649,
    SPELL_VANISH            = 24699,
    SPELL_GOUGE             = 24698,
    SPELL_TRASH             = 3391
};

struct boss_renataki : public CreatureScript
{
    boss_renataki() : CreatureScript("boss_renataki") {}

    struct boss_renatakiAI : public ScriptedAI
    {
        boss_renatakiAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint32 m_uiVanishTimer;
        uint32 m_uiAmbushTimer;
        uint32 m_uiGougeTimer;
        uint32 m_uiThousandBladesTimer;

        void Reset() override
        {
            m_uiVanishTimer = urand(25000, 30000);
            m_uiAmbushTimer = 0;
            m_uiGougeTimer = urand(15000, 25000);
            m_uiThousandBladesTimer = urand(4000, 8000);
        }

        void EnterEvadeMode() override
        {
            // If is vanished, don't evade
            if (m_uiAmbushTimer)
            {
                return;
            }

            ScriptedAI::EnterEvadeMode();
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Note: because the Vanish spell adds invisibility effect on the target, the timers won't be decreased during the vanish phase
            if (m_uiAmbushTimer)
            {
                if (m_uiAmbushTimer <= uiDiff)
                {
                    if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_TRASH) == CAST_OK)
                    {
                        m_uiAmbushTimer = 0;
                    }
                }
                else
                {
                    m_uiAmbushTimer -= uiDiff;
                }

                // don't do anything else while vanished
                return;
            }

            // Invisible_Timer
            if (m_uiVanishTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_VANISH) == CAST_OK)
                {
                    m_uiVanishTimer = urand(25000, 40000);
                    m_uiAmbushTimer = 2000;
                }
            }
            else
            {
                m_uiVanishTimer -= uiDiff;
            }

            // Resetting some aggro so he attacks other gamers
            if (m_uiGougeTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_GOUGE) == CAST_OK)
                {
                    if (m_creature->GetThreatManager().getThreat(m_creature->getVictim()))
                    {
                        m_creature->GetThreatManager().modifyThreatPercent(m_creature->getVictim(), -50);
                    }

                    m_uiGougeTimer = urand(7000, 20000);
                }
            }
            else
            {
                m_uiGougeTimer -= uiDiff;
            }

            // Thausand Blades
            if (m_uiThousandBladesTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_THOUSAND_BLADES) == CAST_OK)
                {
                    m_uiThousandBladesTimer = urand(7000, 12000);
                }
            }
            else
            {
                m_uiThousandBladesTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_renatakiAI(pCreature);
    }
};

void AddSC_boss_renataki()
{
    Script* s;
    s = new boss_renataki();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_renataki";
    //pNewScript->GetAI = &GetAI_boss_renataki;
    //pNewScript->RegisterSelf();
}
