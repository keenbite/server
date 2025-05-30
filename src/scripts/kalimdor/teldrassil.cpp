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
 * SDName:      Teldrassil
 * SD%Complete: 100
 * SDComment:   Quest support: 938
 * SDCategory:  Teldrassil
 * EndScriptData
 */

/**
 * ContentData
 * npc_mist
 * EndContentData
 */

#include "follower_ai.h"

/*####
# npc_mist
####*/

enum
{
    SAY_AT_HOME             = -1000323,
    EMOTE_AT_HOME           = -1000324,
    QUEST_MIST              = 938,
    NPC_ARYNIA              = 3519,
    FACTION_DARNASSUS       = 79
};

struct npc_mist : public CreatureScript
{
    npc_mist() : CreatureScript("npc_mist") {}

    struct npc_mistAI : public FollowerAI
    {
        npc_mistAI(Creature* pCreature) : FollowerAI(pCreature) { }

        void MoveInLineOfSight(Unit* pWho) override
        {
            FollowerAI::MoveInLineOfSight(pWho);

            if (!m_creature->getVictim() && !HasFollowState(STATE_FOLLOW_COMPLETE) && pWho->GetEntry() == NPC_ARYNIA)
            {
                if (m_creature->IsWithinDistInMap(pWho, 10.0f))
                {
                    DoScriptText(SAY_AT_HOME, pWho);
                    DoComplete();
                }
            }
        }

        void DoComplete()
        {
            DoScriptText(EMOTE_AT_HOME, m_creature);

            if (Player* pPlayer = GetLeaderForFollower())
            {
                if (pPlayer->GetQuestStatus(QUEST_MIST) == QUEST_STATUS_INCOMPLETE)
                {
                    pPlayer->GroupEventHappens(QUEST_MIST, m_creature);
                }
            }

            // The follow is over (and for later development, run off to the woods before really end)
            SetFollowComplete();
        }

        // call not needed here, no known abilities
        /*void UpdateFollowerAI(const uint32 uiDiff) override
        {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
        {
            return;
        }

        DoMeleeAttackIfReady();
        }*/
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_mistAI(pCreature);
    }

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_MIST)
        {
            if (npc_mistAI* pMistAI = dynamic_cast<npc_mistAI*>(pCreature->AI()))
            {
                pMistAI->StartFollow(pPlayer, FACTION_DARNASSUS, pQuest);
            }
            return true;
        }

        return false;
    }
};

void AddSC_teldrassil()
{
    Script* s;
    s = new npc_mist();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_mist";
    //pNewScript->GetAI = &GetAI_npc_mist;
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_mist;
    //pNewScript->RegisterSelf();
}
