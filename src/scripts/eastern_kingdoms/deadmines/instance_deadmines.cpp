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
 * SDName:      Instance_Deadmines
 * SD%Complete: 0
 * SDComment:   Placeholder
 * SDCategory:  Deadmines
 * EndScriptData
 */

#include "deadmines.h"

struct is_deadmines : public InstanceScript
{
    is_deadmines() : InstanceScript("instance_deadmines") {}

    class instance_deadmines : public ScriptedInstance
    {
    public:
        instance_deadmines(Map* pMap) : ScriptedInstance(pMap),
            m_uiIronDoorTimer(0),
            m_uiDoorStep(0)
        {
            Initialize();
        }

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        void OnPlayerEnter(Player* pPlayer) override
        {
            // Respawn the Mysterious chest if one of the players who enter the instance has the quest in his log
            if (pPlayer->GetQuestStatus(QUEST_FORTUNE_AWAITS) == QUEST_STATUS_COMPLETE &&
                !pPlayer->GetQuestRewardStatus(QUEST_FORTUNE_AWAITS))
            {
                DoRespawnGameObject(GO_MYSTERIOUS_CHEST, HOUR);
            }
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            if (pCreature->GetEntry() == NPC_MR_SMITE)
            {
                m_mNpcEntryGuidStore[NPC_MR_SMITE] = pCreature->GetObjectGuid();
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_FACTORY_DOOR:
                if (m_auiEncounter[TYPE_RHAHKZOR] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }

                break;
            case GO_MAST_ROOM_DOOR:
                if (m_auiEncounter[TYPE_SNEED] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }

                break;
            case GO_FOUNDRY_DOOR:
                if (m_auiEncounter[TYPE_GILNID] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }

                break;
            case GO_IRON_CLAD_DOOR:
                if (m_auiEncounter[TYPE_IRON_CLAD_DOOR] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                }

                break;
            case GO_DEFIAS_CANNON:
            case GO_SMITE_CHEST:
            case GO_MYSTERIOUS_CHEST:
                break;

            default:
                return;
            }

            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_RHAHKZOR:
                SetData(TYPE_RHAHKZOR, DONE);
                break;
            case NPC_SNEED:
                SetData(TYPE_SNEED, DONE);
                break;
            case NPC_GILNID:
                SetData(TYPE_GILNID, DONE);
                break;
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_RHAHKZOR:
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_FACTORY_DOOR);
                }
                break;
            case TYPE_SNEED:
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_MAST_ROOM_DOOR);
                }
                break;
            case TYPE_GILNID:
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_FOUNDRY_DOOR);
                }
                break;
            case TYPE_IRON_CLAD_DOOR:
                // delayed door animation to sync with Defias Cannon animation
                if (uiData == DONE)
                {
                    m_uiIronDoorTimer = 500;
                }
                break;
            }

            m_auiEncounter[uiType] = uiData;

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " " << m_auiEncounter[3];

                m_strInstData = saveStream.str();

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        uint32 GetData(uint32 uiType) const override
        {
            if (uiType < MAX_ENCOUNTER)
            {
                return m_auiEncounter[uiType];
            }

            return 0;
        }

        const char* Save() const override { return m_strInstData.c_str(); }
        void Load(const char* chrIn) override
        {
            if (!chrIn)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(chrIn);

            std::istringstream loadStream(chrIn);
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >> m_auiEncounter[3];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                {
                    m_auiEncounter[i] = NOT_STARTED;
                }
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        void Update(uint32 uiDiff) override
        {
            if (m_uiIronDoorTimer)
            {
                if (m_uiIronDoorTimer <= uiDiff)
                {
                    switch (m_uiDoorStep)
                    {
                    case 0:
                        DoUseDoorOrButton(GO_IRON_CLAD_DOOR, 0, true);

                        if (Creature* pMrSmite = GetSingleCreatureFromStorage(NPC_MR_SMITE))
                        {
                            DoScriptText(INST_SAY_ALARM1, pMrSmite);
                        }

                        if (GameObject* pDoor = GetSingleGameObjectFromStorage(GO_IRON_CLAD_DOOR))
                        {
                            // should be static spawns, fetch the closest ones at the pier
                            if (Creature* pi1 = GetClosestCreatureWithEntry(pDoor, NPC_PIRATE, 40.0f))
                            {
                                pi1->SetWalk(false);
                                pi1->GetMotionMaster()->MovePoint(0, pDoor->GetPositionX(), pDoor->GetPositionY(), pDoor->GetPositionZ());
                            }

                            if (Creature* pi2 = GetClosestCreatureWithEntry(pDoor, NPC_SQUALLSHAPER, 40.0f))
                            {
                                pi2->SetWalk(false);
                                pi2->GetMotionMaster()->MovePoint(0, pDoor->GetPositionX(), pDoor->GetPositionY(), pDoor->GetPositionZ());
                            }
                        }

                        ++m_uiDoorStep;
                        m_uiIronDoorTimer = 15000;
                        break;
                    case 1:
                        if (Creature* pMrSmite = GetSingleCreatureFromStorage(NPC_MR_SMITE))
                        {
                            DoScriptText(INST_SAY_ALARM2, pMrSmite);
                        }

                        m_uiDoorStep = 0;
                        m_uiIronDoorTimer = 0;
                        break;
                    }
                }
                else
                {
                    m_uiIronDoorTimer -= uiDiff;
                }
            }
        }

    private:
        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        uint32 m_uiIronDoorTimer;
        uint32 m_uiDoorStep;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_deadmines(pMap);
    }
};

void AddSC_instance_deadmines()
{
    Script* s;
    s = new is_deadmines();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_deadmines";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_deadmines;
    //pNewScript->RegisterSelf();
}
