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

#ifndef SC_PRECOMPILED_H
#define SC_PRECOMPILED_H

#include "ScriptDevMgr.h"
#include "Object.h"
#include "ObjectGuid.h"
#include "Unit.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "GameObject.h"
#include "ScriptedCreature_AI.h"
#include "ScriptedGossip.h"
#include "GridSearchers.h"
#include "ScriptedInstance.h"
#include "SpellAuras.h"
#include "World.h"

// sc_gossip.h:             ADD_GOSSIP_ITEM_EXTENDED outcommented box-money (Required until professions are fixed)
// sc_creature.cpp:         Used in ScriptedAI::SelectSpell, outcommented SchoolMask

#endif
