/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
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

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Path.h"
#include "WaypointMovementGenerator.h"

void WorldSession::HandleTaxiNodeStatusQueryOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received opcode CMSG_TAXINODE_STATUS_QUERY");

    ObjectGuid guid;

    recv_data >> guid;
    SendTaxiStatus(guid);
}

void WorldSession::SendTaxiStatus(ObjectGuid guid)
{
    // cheating checks
    Creature* unit = _player->GetMap()->GetCreature(guid);
    if (!unit)
    {
        DEBUG_LOG("WorldSession::SendTaxiStatus - %s not found or you can't interact with it.", guid.GetString().c_str());
        return;
    }

    uint32 curloc = sObjectMgr.GetNearestTaxiNode(unit->GetPositionX(), unit->GetPositionY(), unit->GetPositionZ(), unit->GetMapId(), _player->GetTeam());

    // not found nearest
    if (curloc == 0)
    {
        return;
    }

    DEBUG_LOG("WORLD: current location %u ", curloc);

    WorldPacket data(SMSG_TAXINODE_STATUS, 9);
    data << ObjectGuid(guid);
    data << uint8(_player->m_taxi.IsTaximaskNodeKnown(curloc) ? 1 : 0);
    SendPacket(&data);

    DEBUG_LOG("WORLD: Sent SMSG_TAXINODE_STATUS");
}

void WorldSession::HandleTaxiQueryAvailableNodes(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received opcode CMSG_TAXIQUERYAVAILABLENODES");

    ObjectGuid guid;
    recv_data >> guid;

    // cheating checks
    Creature* unit = _player->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_FLIGHTMASTER);
    if (!unit)
    {
        DEBUG_LOG("WORLD: HandleTaxiQueryAvailableNodes - %s not found or you can't interact with him.", guid.GetString().c_str());
        return;
    }

    // remove fake death
    if (_player->hasUnitState(UNIT_STAT_DIED))
    {
        _player->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);
    }

    // unknown taxi node case
    if (SendLearnNewTaxiNode(unit))
    {
        return;
    }

    // known taxi node case
    SendTaxiMenu(unit);
}

void WorldSession::SendTaxiMenu(Creature* unit)
{
    // find current node
    uint32 curloc = sObjectMgr.GetNearestTaxiNode(unit->GetPositionX(), unit->GetPositionY(), unit->GetPositionZ(), unit->GetMapId(), _player->GetTeam());

    if (curloc == 0)
    {
        return;
    }

    DEBUG_LOG("WORLD: CMSG_TAXINODE_STATUS_QUERY %u ", curloc);

    WorldPacket data(SMSG_SHOWTAXINODES, (4 + 8 + 4 + 8 * 4));
    data << uint32(1);
    data << unit->GetObjectGuid();
    data << uint32(curloc);
    _player->m_taxi.AppendTaximaskTo(data, _player->IsTaxiCheater());
    SendPacket(&data);

    DEBUG_LOG("WORLD: Sent SMSG_SHOWTAXINODES");
}

void WorldSession::SendDoFlight(uint32 mountDisplayId, uint32 path, uint32 pathNode)
{
    // remove fake death
    if (_player->hasUnitState(UNIT_STAT_DIED))
    {
        _player->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);
    }

    while (_player->GetMotionMaster()->GetCurrentMovementGeneratorType() == FLIGHT_MOTION_TYPE)
    {
        _player->GetMotionMaster()->MovementExpired(false);
    }

    if (mountDisplayId)
    {
        _player->Mount(mountDisplayId);
    }

    _player->GetMotionMaster()->MoveTaxiFlight(path, pathNode);
}

bool WorldSession::SendLearnNewTaxiNode(Creature* unit)
{
    // find current node
    uint32 curloc = sObjectMgr.GetNearestTaxiNode(unit->GetPositionX(), unit->GetPositionY(), unit->GetPositionZ(), unit->GetMapId(), _player->GetTeam());

    if (curloc == 0)
    {
        return true;                                         // `true` send to avoid WorldSession::SendTaxiMenu call with one more curlock seartch with same false result.
    }

    if (_player->m_taxi.SetTaximaskNode(curloc))
    {
        WorldPacket msg(SMSG_NEW_TAXI_PATH, 0);
        SendPacket(&msg);

        WorldPacket update(SMSG_TAXINODE_STATUS, 9);
        update << ObjectGuid(unit->GetObjectGuid());
        update << uint8(1);
        SendPacket(&update);

        return true;
    }
    else
    {
        return false;
    }
}

void WorldSession::SendActivateTaxiReply(ActivateTaxiReply reply)
{
    WorldPacket data(SMSG_ACTIVATETAXIREPLY, 4);
    data << uint32(reply);
    SendPacket(&data);

    DEBUG_LOG("WORLD: Sent SMSG_ACTIVATETAXIREPLY");
}

void WorldSession::HandleActivateTaxiExpressOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received opcode CMSG_ACTIVATETAXIEXPRESS");

    ObjectGuid guid;
    uint32 node_count, _totalcost;

    recv_data >> guid >> _totalcost >> node_count;

    Creature* npc = _player->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_FLIGHTMASTER);
    if (!npc)
    {
        DEBUG_LOG("WORLD: HandleActivateTaxiExpressOpcode - %s not found or you can't interact with it.", guid.GetString().c_str());
        return;
    }
    std::vector<uint32> nodes;

    for (uint32 i = 0; i < node_count; ++i)
    {
        uint32 node;
        recv_data >> node;

        if (!_player->m_taxi.IsTaximaskNodeKnown(node) && !_player->IsTaxiCheater())
        {
            SendActivateTaxiReply(ERR_TAXINOTVISITED);
            recv_data.rpos(recv_data.wpos()); // prevent additional spam at rejected packet
            return;
        }
        nodes.push_back(node);
    }

    if (nodes.empty())
    {
        return;
    }

    DEBUG_LOG("WORLD: Received opcode CMSG_ACTIVATETAXIEXPRESS from %d to %d" , nodes.front(), nodes.back());

    _player->ActivateTaxiPathTo(nodes, npc);
}

void WorldSession::HandleMoveSplineDoneOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received opcode CMSG_MOVE_SPLINE_DONE");

    MovementInfo movementInfo;                              // used only for proper packet read

    recv_data >> movementInfo;
    recv_data >> Unused<uint32>();                          // unk
    recv_data >> Unused<uint32>();                          // unk2


    // in taxi flight packet received in 2 case:
    // 1) end taxi path in far (multi-node) flight
    // 2) switch from one map to other in case multi-map taxi path
    // we need process only (1)
    uint32 curDest = _player->m_taxi.GetTaxiDestination();
    if (!curDest)
    {
        return;
    }

    TaxiNodesEntry const* curDestNode = sTaxiNodesStore.LookupEntry(curDest);

    // far teleport case
    if (curDestNode && curDestNode->map_id != _player->GetMapId())
    {
        if (_player->GetMotionMaster()->GetCurrentMovementGeneratorType() == FLIGHT_MOTION_TYPE)
        {
            // short preparations to continue flight
            FlightPathMovementGenerator* flight = (FlightPathMovementGenerator*)(_player->GetMotionMaster()->top());

            flight->Interrupt(*_player);                // will reset at map landing

            flight->SetCurrentNodeAfterTeleport();
            TaxiPathNodeEntry const& node = flight->GetPath()[flight->GetCurrentNode()];
            flight->SkipCurrentNode();

            _player->TeleportTo(curDestNode->map_id, node.x, node.y, node.z, _player->GetOrientation());
        }
        return;
    }

    uint32 destinationnode = _player->m_taxi.NextTaxiDestination();
    if (destinationnode > 0)                                // if more destinations to go
    {
        // current source node for next destination
        uint32 sourcenode = _player->m_taxi.GetTaxiSource();

        // Add to taximask middle hubs in taxicheat mode (to prevent having player with disabled taxicheat and not having back flight path)
        if (_player->IsTaxiCheater())
        {
            if (_player->m_taxi.SetTaximaskNode(sourcenode))
            {
                WorldPacket data(SMSG_NEW_TAXI_PATH, 0);
                _player->GetSession()->SendPacket(&data);
            }
        }

        DEBUG_LOG("WORLD: Taxi has to go from %u to %u", sourcenode, destinationnode);

        uint32 mountDisplayId = sObjectMgr.GetTaxiMountDisplayId(sourcenode, _player->GetTeam());

        uint32 path, cost;
        sObjectMgr.GetTaxiPath(sourcenode, destinationnode, path, cost);

        if (path && mountDisplayId)
        {
            SendDoFlight(mountDisplayId, path, 1);           // skip start fly node
        }
        else
        {
            _player->m_taxi.ClearTaxiDestinations();     // clear problematic path and next
        }
    }
    else
    {
        _player->m_taxi.ClearTaxiDestinations();         // not destinations, clear source node
    }
}

void WorldSession::HandleActivateTaxiOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received opcode CMSG_ACTIVATETAXI");

    ObjectGuid guid;
    std::vector<uint32> nodes;
    nodes.resize(2);

    recv_data >> guid >> nodes[0] >> nodes[1];
    DEBUG_LOG("WORLD: Received opcode CMSG_ACTIVATETAXI from %d to %d" , nodes[0], nodes[1]);
    Creature* npc = _player->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_FLIGHTMASTER);
    if (!npc)
    {
        DEBUG_LOG("WORLD: HandleActivateTaxiOpcode - %s not found or you can't interact with it.", guid.GetString().c_str());
        return;
    }
    if (!_player->IsTaxiCheater())
    {
        if (!_player->m_taxi.IsTaximaskNodeKnown(nodes[0]) || !_player->m_taxi.IsTaximaskNodeKnown(nodes[1]))
        {
            SendActivateTaxiReply(ERR_TAXINOTVISITED);
            return;
        }
    }
    _player->ActivateTaxiPathTo(nodes, npc);
}
