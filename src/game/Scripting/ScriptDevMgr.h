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

#ifndef SC_SCRIPTMGR_H
#define SC_SCRIPTMGR_H

#include "Common.h"
#include "DBCStructure.h"
#include "QuestDef.h"
#include "ScriptMgr.h"

class Player;
class Creature;
class CreatureAI;
class InstanceData;
class Quest;
class Item;
class GameObject;
class GameObjectAI;
class SpellCastTargets;
class Map;
class Unit;
class WorldObject;
class Aura;
class Object;
class ObjectGuid;

// *********************************************************
// **************** Functions used by core *****************

class SD3
{
public:
    static void FreeScriptLibrary();
    static void InitScriptLibrary();
    static char const* GetScriptLibraryVersion();

    static CreatureAI* GetCreatureAI(Creature* pCreature);
    static GameObjectAI* GetGameObjectAI(GameObject* pGo);
    static InstanceData* CreateInstanceData(Map* pMap);

    static bool GossipHello(Player*, Creature*);
    static bool GOGossipHello(Player*, GameObject*);
    static bool ItemGossipHello(Player*, Item*);
    static bool GossipSelect(Player*, Creature*, uint32, uint32);
    static bool GOGossipSelect(Player*, GameObject*, uint32, uint32);
    static bool ItemGossipSelect(Player*, Item*, uint32, uint32);
    static bool GossipSelectWithCode(Player*, Creature*, uint32, uint32, const char*);
    static bool GOGossipSelectWithCode(Player*, GameObject*, uint32, uint32, const char*);
    static bool ItemGossipSelectWithCode(Player*, Item*, uint32, uint32, const char*);
    static bool QuestAccept(Player*, Creature*, Quest const*);
    static bool GOQuestAccept(Player*, GameObject*, Quest const*);
    static bool ItemQuestAccept(Player*, Item*, Quest const*);
    static bool QuestRewarded(Player*, Creature*, Quest const*);
    static bool GOQuestRewarded(Player*, GameObject*, Quest const*);
    static uint32 GetNPCDialogStatus(Player*, Creature*);
    static uint32 GetGODialogStatus(Player*, GameObject*);
    static bool GOUse(Player*, GameObject*);
    static bool GOUse(Unit*, GameObject*);
    static bool ItemUse(Player*, Item*, SpellCastTargets const&);
    static bool ItemEquip(Player*, Item*, bool);    //new TODO
    static bool ItemDelete(Player*, Item*);         //new TODO
    static bool AreaTrigger(Player*, AreaTriggerEntry const*);
    static bool NpcSpellClick(Player* pPlayer, Creature* pClickedCreature, uint32 uiSpellId);
    static bool ProcessEvent(uint32, Object*, Object*, bool);
    static bool EffectDummyUnit(Unit*, uint32, SpellEffectIndex, Unit*, ObjectGuid);
    static bool EffectDummyGameObject(Unit*, uint32, SpellEffectIndex, GameObject*, ObjectGuid);
    static bool EffectDummyItem(Unit*, uint32, SpellEffectIndex, Item*, ObjectGuid);
    static bool EffectScriptEffectUnit(Unit*, uint32, SpellEffectIndex, Unit*, ObjectGuid);
    static bool AuraDummy(Aura const *, bool);
    //static bool AuraDummyTick(Aura const*);         [-ZERO:] no dummy ticks. TODO
};

// *********************************************************
// ************** Some defines used globally ***************

// Basic defines
#define VISIBLE_RANGE       (166.0f)                        // MAX visible range (size of grid)
#define DEFAULT_TEXT        "<ScriptDev3 Text Entry Missing!>"

/* Escort Factions
 * TODO: find better namings and definitions.
 * N=Neutral, A=Alliance, H=Horde.
 * NEUTRAL or FRIEND = Hostility to player surroundings (not a good definition)
 * ACTIVE or PASSIVE = Hostility to environment surroundings.
 */
enum EscortFaction
{
    FACTION_ESCORT_A_NEUTRAL_PASSIVE    = 10,
    FACTION_ESCORT_H_NEUTRAL_PASSIVE    = 33,
    FACTION_ESCORT_N_NEUTRAL_PASSIVE    = 113,

    FACTION_ESCORT_A_NEUTRAL_ACTIVE     = 231,
    FACTION_ESCORT_H_NEUTRAL_ACTIVE     = 232,
    FACTION_ESCORT_N_NEUTRAL_ACTIVE     = 250,

    FACTION_ESCORT_N_FRIEND_PASSIVE     = 290,
    FACTION_ESCORT_N_FRIEND_ACTIVE      = 495,

    FACTION_ESCORT_A_PASSIVE            = 774,
    FACTION_ESCORT_H_PASSIVE            = 775,

    FACTION_ESCORT_N_ACTIVE             = 1986,
    FACTION_ESCORT_H_ACTIVE             = 2046
};

// *********************************************************
// ************* Some structures used globally *************
struct CreatureScript;
struct GameObjectScript;
struct ItemScript;
struct AreaTriggerScript;
struct MapEventScript;
struct ZoneScript;
struct OutdoorPvPScript;
struct BattleGroundScript;
struct InstanceScript;
struct SpellScript;
struct AuraScript;
struct ConditionScript;
struct AchievementScript;

/**
 * @brief Base structure for all script types.
 */
struct Script
{
    std::string Name;  ///< Name of the script.
    ScriptedObjectType Type;  ///< Type of the scripted object.

    /**
     * @brief Registers the script.
     * @param bReportError Whether to report an error if registration fails.
     */
    void RegisterSelf(bool bReportError = true);

    /**
     * @brief Checks if the script is valid.
     * @return True if the script is valid, false otherwise.
     */
    virtual bool IsValid() { return true; }

    Script() : Name(""), Type(SCRIPTED_MAX_TYPE) {}
    Script(ScriptedObjectType type, const char* name) : Name(name), Type(type) {}

    CreatureScript* ToCreatureScript() { return Type == SCRIPTED_UNIT && IsValid() ? (CreatureScript*)this : nullptr; }
    GameObjectScript* ToGameObjectScript() { return Type == SCRIPTED_GAMEOBJECT && IsValid() ? (GameObjectScript*)this : nullptr; }
    ItemScript* ToItemScript() { return Type == SCRIPTED_ITEM && IsValid() ? (ItemScript*)this : nullptr; }
    AreaTriggerScript* ToAreaTriggerScript() { return Type == SCRIPTED_AREATRIGGER && IsValid() ? (AreaTriggerScript*)this : nullptr; }
    MapEventScript* ToMapEventScript() { return Type == SCRIPTED_MAPEVENT && IsValid() ? (MapEventScript*)this : nullptr; }
    ZoneScript* ToZoneScript() { return Type == SCRIPTED_MAP && IsValid() ? (ZoneScript*)this : nullptr; }
    OutdoorPvPScript* ToOutdoorPvPScript() { return Type == SCRIPTED_PVP_ZONE && IsValid() ? (OutdoorPvPScript*)this : nullptr; }
    BattleGroundScript* ToBattleGroundScript() { return Type == SCRIPTED_BATTLEGROUND && IsValid() ? (BattleGroundScript*)this : nullptr; }
    InstanceScript* ToInstanceScript() { return Type == SCRIPTED_INSTANCE && IsValid() ? (InstanceScript*)this : nullptr; }
    SpellScript* ToSpellScript() { return Type == SCRIPTED_SPELL && IsValid() ? (SpellScript*)this : nullptr; }
    AuraScript* ToAuraScript() { return Type == SCRIPTED_AURASPELL && IsValid() ? (AuraScript*)this : nullptr; }
    ConditionScript* ToConditionScript() { return Type == SCRIPTED_CONDITION && IsValid() ? (ConditionScript*)this : nullptr; }
    AchievementScript* ToAchievementScript() { return Type == SCRIPTED_ACHIEVEMENT && IsValid() ? (AchievementScript*)this : nullptr; }
};

/**
 * @brief Structure for creature scripts.
 */
struct CreatureScript : public Script
{
    CreatureScript(const char* name) : Script(SCRIPTED_UNIT, name) {}

    virtual bool OnGossipHello(Player*, Creature*) { return false; }
    virtual bool OnGossipSelect(Player*, Creature*, uint32, uint32) { return false; }
    virtual bool OnGossipSelectWithCode(Player*, Creature*, uint32, uint32, const char*) { return false; }
    virtual uint32 OnDialogEnd(Player*, Creature*) { return DIALOG_STATUS_UNDEFINED; }
    virtual bool OnQuestAccept(Player*, Creature*, Quest const*) { return false; }
    virtual bool OnQuestRewarded(Player*, Creature*, Quest const*) { return false; }
    virtual bool OnSpellClick(Player*, Creature*, uint32) { return false; }

    virtual CreatureAI* GetAI(Creature*) { return nullptr; }
};

/**
 * @brief Structure for game object scripts.
 */
struct GameObjectScript : public Script
{
    GameObjectScript(const char* name) : Script(SCRIPTED_GAMEOBJECT, name) {}

    virtual bool OnGossipHello(Player*, GameObject*) { return false; }
    virtual bool OnGossipSelect(Player*, GameObject*, uint32, uint32) { return false; }
    virtual bool OnGossipSelectWithCode(Player*, GameObject*, uint32, uint32, const char*) { return false; }
    virtual uint32 OnDialogEnd(Player*, GameObject*) { return DIALOG_STATUS_UNDEFINED; }
    virtual bool OnQuestAccept(Player*, GameObject*, Quest const*) { return false; }
    virtual bool OnQuestRewarded(Player*, GameObject*, Quest const*) { return false; }
    virtual bool OnUse(Player*, GameObject*) { return false; }
    virtual bool OnUse(Unit*, GameObject*) { return false; }

    virtual GameObjectAI* GetAI(GameObject*) { return nullptr; }
};

/**
 * @brief Structure for item scripts.
 */
struct ItemScript : public Script
{
    ItemScript(const char* name) : Script(SCRIPTED_ITEM, name) {}

    virtual bool OnQuestAccept(Player*, Item*, Quest const*) { return false; }
    virtual bool OnUse(Player*, Item*, SpellCastTargets const&) { return false; }
    virtual bool OnEquip(Player*, Item*, bool on) { return false; }
    virtual bool OnDelete(Player*, Item*) { return false; }
    virtual bool OnGossipHello(Player*, Item*) { return false; }
    virtual bool OnGossipSelect(Player*, Item*, uint32, uint32) { return false; }
    virtual bool OnGossipSelectWithCode(Player*, Item*, uint32, uint32, const char*) { return false; }
};

/**
 * @brief Structure for area trigger scripts.
 */
struct AreaTriggerScript : public Script
{
    AreaTriggerScript(const char* name) : Script(SCRIPTED_AREATRIGGER, name) {}

    virtual bool OnTrigger(Player*, AreaTriggerEntry const*) { return false; }
};

/**
 * @brief Structure for map event scripts.
 */
struct MapEventScript : public Script
{
    MapEventScript(const char *name) : Script(SCRIPTED_MAPEVENT, name) {}

    virtual bool OnReceived(uint32, Object*, Object*, bool) { return false; }
};

/**
 * @brief Structure for zone scripts.
 */
struct ZoneScript : public Script
{
    ZoneScript(const char* name) : Script(SCRIPTED_MAP, name) {}
    ZoneScript(const char* name, ScriptedObjectType type) : Script(type, name) {}

    virtual bool OnMapEvent(uint32, Object*, Object*, bool) { return false; }

    virtual InstanceData* GetInstanceData(Map*) { return nullptr; }
};

/**
 * @brief Structure for outdoor PvP scripts.
 */
struct OutdoorPvPScript : public ZoneScript
{
    OutdoorPvPScript(const char* name) : ZoneScript(name, SCRIPTED_PVP_ZONE) {}
};

/**
 * @brief Structure for battleground scripts.
 */
struct BattleGroundScript : public ZoneScript
{
    BattleGroundScript(const char* name) : ZoneScript(name, SCRIPTED_BATTLEGROUND) {}
    //bool IsValid() override { return map && map->IsBattleGround(); }
};

/**
 * @brief Structure for instance scripts.
 */
struct InstanceScript : public ZoneScript
{
    InstanceScript(const char* name) : ZoneScript(name, SCRIPTED_INSTANCE) {}
    //bool IsValid() override { return map && map->IsDungeon(); }
};

/**
 * @brief Structure for spell scripts.
 */
struct SpellScript : public Script
{
    SpellScript(const char* name) : Script(SCRIPTED_SPELL, name) {}
    //bool IsValid() override { return bool(sSpellStore.LookupEntry(spellID)); }

    virtual bool EffectDummy(Unit*, uint32, SpellEffectIndex, Object*, ObjectGuid) { return false; }
    virtual bool EffectScriptEffect(Unit*, uint32, SpellEffectIndex, Unit*, ObjectGuid) { return false; }
};

/**
 * @brief Structure for aura scripts.
 */
struct AuraScript : public Script
{
    AuraScript(const char* name) : Script(SCRIPTED_AURASPELL, name) {}
    //bool IsValid() override { return bool(sSpellStore.LookupEntry(spellID)); }

    virtual bool OnDummyApply(const Aura*, bool) { return false; }
#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
    virtual bool OnDummyTick(const Aura*) { return false; }
#endif
};

/**
 * @brief Structure for condition scripts.
 */
struct ConditionScript : public Script
{
    ConditionScript(const char* name) : Script(SCRIPTED_CONDITION, name) {}
};

/**
 * @brief Structure for achievement scripts.
 */
struct AchievementScript : public Script
{
    AchievementScript(const char* name) : Script(SCRIPTED_ACHIEVEMENT, name) {}
};

class PlayerScript : public Script
{
public:
    PlayerScript(const char* name) : Script(SCRIPTED_PLAYERSCRIPT, name) {}
    virtual ~PlayerScript() = default;

    // Called when a player kills another player
    virtual void OnPVPKill(Player* killer, Player* killed);

    // Called when a player kills a creature
    virtual void OnCreatureKill(Player* killer, Creature* killed);

    // Called when a player is killed by a creature
    virtual void OnPlayerKilledByCreature(Creature* killer, Player* killed);

    // Called when a player's level changes (right before the level is applied)
    virtual void OnLevelChanged(Player* player, uint8 newLevel);

    // Called when a player's free talent points change (right before the change is applied)
    virtual void OnFreeTalentPointsChanged(Player* player, uint32 points);

    // Called when a player's talent points are reset (right before the reset is done)
    virtual void OnTalentsReset(Player* player, bool no_cost);

    // Called when a player switches to a new zone
    virtual void OnUpdateZone(Player* player, uint32 newZone, uint32 newArea);
};

// *********************************************************
// ************* Some functions used globally **************

/**
 * @brief Generic scripting text function.
 * @param iTextEntry Text entry ID.
 * @param pSource Source of the text.
 * @param pTarget Target of the text (optional).
 */
void DoScriptText(int32 iTextEntry, WorldObject* pSource, Unit* pTarget = nullptr);

/**
 * @brief Simulates script text for a map.
 * @param iTextEntry Text entry ID.
 * @param uiCreatureEntry Creature entry ID.
 * @param pMap Pointer to the map.
 * @param pCreatureSource Source creature (optional).
 * @param pTarget Target unit (optional).
 */
void DoOrSimulateScriptTextForMap(int32 iTextEntry, uint32 uiCreatureEntry, Map* pMap, Creature* pCreatureSource = nullptr, Unit* pTarget = nullptr);

#endif
