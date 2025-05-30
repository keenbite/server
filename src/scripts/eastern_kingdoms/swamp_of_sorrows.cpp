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
 * SDName:      Swamp_of_Sorrows
 * SD%Complete: 100
 * SDComment:   Quest support: 1393.
 * SDCategory:  Swap of Sorrows
 * EndScriptData
 */

/**
 * ContentData
 * npc_galen_goodward
 * EndContentData
 */

#include "escort_ai.h"

/*######
## npc_galen_goodward
######*/

enum Galen
{
    QUEST_GALENS_ESCAPE     = 1393,

    GO_GALENS_CAGE          = 37118,

    SAY_PERIODIC            = -1000582,
    SAY_QUEST_ACCEPTED      = -1000583,
    SAY_ATTACKED_1          = -1000584,
    SAY_ATTACKED_2          = -1000585,
    SAY_QUEST_COMPLETE      = -1000586,
    EMOTE_WHISPER           = -1000587,
    EMOTE_DISAPPEAR         = -1000588
};

struct npc_galen_goodward : public CreatureScript
{
    npc_galen_goodward() : CreatureScript("npc_galen_goodward") {}

    struct npc_galen_goodwardAI : public npc_escortAI
    {
        npc_galen_goodwardAI(Creature* pCreature) : npc_escortAI(pCreature) { }

        ObjectGuid m_galensCageGuid;
        uint32 m_uiPeriodicSay;

        void Reset() override
        {
            m_uiPeriodicSay = 6000;
        }

        void Aggro(Unit* pWho) override
        {
            if (HasEscortState(STATE_ESCORT_ESCORTING))
            {
                DoScriptText(urand(0, 1) ? SAY_ATTACKED_1 : SAY_ATTACKED_2, m_creature, pWho);
            }
        }

        void WaypointStart(uint32 uiPointId) override
        {
            switch (uiPointId)
            {
            case 0:
            {
                      GameObject* pCage = nullptr;
                      if (m_galensCageGuid)
                      {
                          pCage = m_creature->GetMap()->GetGameObject(m_galensCageGuid);
                      }
                      else
                      {
                          pCage = GetClosestGameObjectWithEntry(m_creature, GO_GALENS_CAGE, INTERACTION_DISTANCE);
                      }

                      if (pCage)
                      {
                          pCage->UseDoorOrButton();
                          m_galensCageGuid = pCage->GetObjectGuid();
                      }
                      break;
            }
            case 21:
                DoScriptText(EMOTE_DISAPPEAR, m_creature);
                break;
            }
        }

        void WaypointReached(uint32 uiPointId) override
        {
            switch (uiPointId)
            {
            case 0:
                if (GameObject* pCage = m_creature->GetMap()->GetGameObject(m_galensCageGuid))
                {
                    pCage->ResetDoorOrButton();
                }
                break;
            case 20:
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    m_creature->SetFacingToObject(pPlayer);
                    DoScriptText(SAY_QUEST_COMPLETE, m_creature, pPlayer);
                    DoScriptText(EMOTE_WHISPER, m_creature, pPlayer);
                    pPlayer->GroupEventHappens(QUEST_GALENS_ESCAPE, m_creature);
                }
                SetRun(true);
                break;
            }
        }

        void UpdateEscortAI(const uint32 uiDiff) override
        {

            if (m_uiPeriodicSay < uiDiff)
            {
                if (HasEscortState(STATE_ESCORT_NONE))
                {
                    DoScriptText(SAY_PERIODIC, m_creature);
                }
                m_uiPeriodicSay = 6000;
            }
            else
            {
                m_uiPeriodicSay -= uiDiff;
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            DoMeleeAttackIfReady();
        }
    };

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_GALENS_ESCAPE)
        {

            if (npc_galen_goodwardAI* pEscortAI = dynamic_cast<npc_galen_goodwardAI*>(pCreature->AI()))
            {
                pEscortAI->Start(false, pPlayer, pQuest);
                pCreature->SetFactionTemporary(FACTION_ESCORT_N_NEUTRAL_ACTIVE, TEMPFACTION_RESTORE_RESPAWN);
                DoScriptText(SAY_QUEST_ACCEPTED, pCreature);
            }
        }
        return true;
    }

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_galen_goodwardAI(pCreature);
    }
};

void AddSC_swamp_of_sorrows()
{
    Script* s;
    s = new npc_galen_goodward();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_galen_goodward";
    //pNewScript->GetAI = &GetAI_npc_galen_goodward;
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_galen_goodward;
    //pNewScript->RegisterSelf();
}
