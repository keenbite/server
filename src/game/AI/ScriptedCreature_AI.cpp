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

#include "Cell.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "ScriptDevMgr.h"
#include "ScriptedCreature_AI.h"
#include "Spell.h"
#include "WorldPacket.h"

// Spell summary for ScriptedAI::SelectSpell
struct TSpellSummary
{
    uint8 Targets;  // set of enum SelectTarget
    uint8 Effects;  // set of enum SelectEffect
}* SpellSummary;

/**
 * @brief Constructor for ScriptedAI.
 * @param pCreature Pointer to the creature.
 */
ScriptedAI::ScriptedAI(Creature* pCreature) : CreatureAI(pCreature),
    m_uiEvadeCheckCooldown(2500)
{}

/// This function shows if combat movement is enabled, overwrite for more info
void ScriptedAI::GetAIInformation(ChatHandler& reader)
{
    reader.PSendSysMessage("ScriptedAI, combat movement is %s", reader.GetOnOffStr(IsCombatMovement()));
}

/// Return if the creature can "see" pWho
bool ScriptedAI::IsVisible(Unit* pWho) const
{
    if (!pWho)
    {
        return false;
    }

    return m_creature->IsWithinDist(pWho, VISIBLE_RANGE) && pWho->IsVisibleForOrDetect(m_creature, m_creature, true);
}

/**
 * @brief Handles the creature's behavior when a unit moves into its line of sight.
 * @param pWho Pointer to the unit that moved into line of sight.
 */
void ScriptedAI::MoveInLineOfSight(Unit* pWho)
{
    if (m_creature->CanInitiateAttack() && pWho->IsTargetableForAttack() &&
        m_creature->IsHostileTo(pWho) && pWho->isInAccessablePlaceFor(m_creature))
    {
        if (!m_creature->CanFly() && m_creature->GetDistanceZ(pWho) > CREATURE_Z_ATTACK_RANGE)
        {
            return;
        }

        if (m_creature->IsWithinDistInMap(pWho, m_creature->GetAttackDistance(pWho)) && m_creature->IsWithinLOSInMap(pWho))
        {
            if (!m_creature->getVictim())
            {
                pWho->RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH);
                AttackStart(pWho);
            }
            else if (m_creature->GetMap()->IsDungeon())
            {
                pWho->SetInCombatWith(m_creature);
                m_creature->AddThreat(pWho);
            }
        }
    }
}

/**
 * @brief Starts the creature's attack on a specified unit.
 * @param pWho Pointer to the unit to attack.
 */
void ScriptedAI::AttackStart(Unit* pWho)
{
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
    if (!m_creature->CanInitiateAttack())
    {
        return;
    }
#endif
    if (pWho && m_creature->Attack(pWho, true))  // The Attack function also uses basic checks if pWho can be attacked
    {
        m_creature->AddThreat(pWho);
        m_creature->SetInCombatWith(pWho);
        pWho->SetInCombatWith(m_creature);

        HandleMovementOnAttackStart(pWho);
    }
}

/**
 * @brief Handles the creature's behavior when entering combat.
 * @param pEnemy Pointer to the enemy unit.
 */
void ScriptedAI::EnterCombat(Unit* pEnemy)
{
    if (pEnemy)
    {
        Aggro(pEnemy);
    }
}

/**
 * @brief Main update function for the AI.
 * @param uiDiff Time since the last update.
 */
void ScriptedAI::UpdateAI(const uint32 /*uiDiff*/)
{
    // Check if we have a current target
    if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
    {
        return;
    }

    DoMeleeAttackIfReady();

#if defined (WOTLK) || defined (CATA) || defined (MISTS)
    Unit* victim = m_creature->getVictim();

    const SpellEntry* potentialSpell = m_creature->ReachWithSpellAttack(victim);
    if (potentialSpell)
    {
        m_creature->CastSpell(victim, potentialSpell->Id, true);
    }
#endif
}

/**
 * @brief Handles the creature's behavior when evading.
 */
void ScriptedAI::EnterEvadeMode()
{
    m_creature->RemoveAllAurasOnEvade();
    m_creature->DeleteThreatList();
    m_creature->CombatStop(true);

    if (m_creature->IsAlive())
    {
        m_creature->GetMotionMaster()->MoveTargetedHome();
    }

    m_creature->SetLootRecipient(nullptr);

    Reset();
}

/// This function calls Reset() to reset variables as expected
void ScriptedAI::JustRespawned()
{
    Reset();
}

/**
 * @brief Starts movement towards a specified victim.
 * @param pVictim Pointer to the victim.
 * @param fDistance Distance to maintain from the victim.
 * @param fAngle Angle to maintain from the victim.
 */
void ScriptedAI::DoStartMovement(Unit* pVictim, float fDistance, float fAngle)
{
    if (pVictim)
    {
        m_creature->GetMotionMaster()->MoveChase(pVictim, fDistance, fAngle);
    }
}

/**
 * @brief Starts no movement towards a specified victim.
 * @param pVictim Pointer to the victim.
 */
void ScriptedAI::DoStartNoMovement(Unit* pVictim)
{
    if (!pVictim)
    {
        return;
    }

    m_creature->GetMotionMaster()->MoveIdle();
    m_creature->StopMoving();
}

/**
 * @brief Stops the creature's attack.
 */
void ScriptedAI::DoStopAttack()
{
    if (m_creature->getVictim())
    {
        m_creature->AttackStop();
    }
}

/**
 * @brief Casts a spell on a specified target.
 * @param pTarget Pointer to the target.
 * @param uiSpellId ID of the spell to cast.
 * @param bTriggered Whether the spell is triggered.
 */
void ScriptedAI::DoCast(Unit* pTarget, uint32 uiSpellId, bool bTriggered)
{
    if (m_creature->IsNonMeleeSpellCasted(false) && !bTriggered)
    {
        return;
    }

    m_creature->CastSpell(pTarget, uiSpellId, bTriggered);
}

/**
 * @brief Casts a spell on a specified target using spell info.
 * @param pTarget Pointer to the target.
 * @param pSpellInfo Pointer to the spell info.
 * @param bTriggered Whether the spell is triggered.
 */
void ScriptedAI::DoCastSpell(Unit* pTarget, SpellEntry const* pSpellInfo, bool bTriggered)
{
    if (m_creature->IsNonMeleeSpellCasted(false) && !bTriggered)
    {
        return;
    }

    m_creature->CastSpell(pTarget, pSpellInfo, bTriggered);
}

/**
 * @brief Plays a sound to all nearby players.
 * @param pSource Pointer to the source object.
 * @param uiSoundId ID of the sound to play.
 */
void ScriptedAI::DoPlaySoundToSet(WorldObject* pSource, uint32 uiSoundId)
{
    if (!pSource)
    {
        return;
    }

    if (!GetSoundEntriesStore()->LookupEntry(uiSoundId))
    {
        script_error_log("Invalid soundId %u used in DoPlaySoundToSet (Source: TypeId %u, GUID %u)", uiSoundId, pSource->GetTypeId(), pSource->GetGUIDLow());
        return;
    }

    pSource->PlayDirectSound(uiSoundId);
}

/**
 * @brief Spawns a creature relative to the current creature.
 * @param uiId ID of the creature to spawn.
 * @param fX X offset from the current creature.
 * @param fY Y offset from the current creature.
 * @param fZ Z offset from the current creature.
 * @param fAngle Angle to spawn the creature at.
 * @param uiType Type of the spawn.
 * @param uiDespawntime Despawn time of the creature.
 * @return Pointer to the spawned creature.
 */
Creature* ScriptedAI::DoSpawnCreature(uint32 uiId, float fX, float fY, float fZ, float fAngle, uint32 uiType, uint32 uiDespawntime)
{
    return m_creature->SummonCreature(uiId, m_creature->GetPositionX() + fX, m_creature->GetPositionY() + fY, m_creature->GetPositionZ() + fZ, fAngle, (TempSpawnType)uiType, uiDespawntime);
}

/**
 * @brief Selects a spell to cast based on specified criteria.
 * @param pTarget Pointer to the target.
 * @param uiSchool School of the spell.
 * @param iMechanic Mechanic of the spell.
 * @param selectTargets Target type of the spell.
 * @param uiPowerCostMin Minimum power cost of the spell.
 * @param uiPowerCostMax Maximum power cost of the spell.
 * @param fRangeMin Minimum range of the spell.
 * @param fRangeMax Maximum range of the spell.
 * @param selectEffects Effect type of the spell.
 * @return Pointer to the selected spell entry.
 */
SpellEntry const* ScriptedAI::SelectSpell(Unit* pTarget, int32 uiSchool, int32 iMechanic, SelectTarget selectTargets, uint32 uiPowerCostMin, uint32 uiPowerCostMax, float fRangeMin, float fRangeMax, SelectEffect selectEffects)
{
    // No target so we can't cast
    if (!pTarget)
    {
        return nullptr;
    }

    // Silenced so we can't cast
    if (m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED))
    {
        return nullptr;
    }

    // Using the extended script system we first create a list of viable spells
    SpellEntry const* apSpell[4];
    memset(apSpell, 0, sizeof(SpellEntry*) * 4);

    uint32 uiSpellCount = 0;

    SpellEntry const* pTempSpell;
    SpellRangeEntry const* pTempRange;

    // Check if each spell is viable(set it to null if not)
    for (uint8 i = 0; i < 4; ++i)
    {
        pTempSpell = GetSpellStore()->LookupEntry(m_creature->m_spells[i]);

        // This spell doesn't exist
        if (!pTempSpell)
        {
            continue;
        }

        // Targets and Effects checked first as most used restrictions
        // Check the spell targets if specified
        if (selectTargets && !(SpellSummary[m_creature->m_spells[i]].Targets & (1 << (selectTargets - 1))))
        {
            continue;
        }

        // Check the type of spell if we are looking for a specific spell type
        if (selectEffects && !(SpellSummary[m_creature->m_spells[i]].Effects & (1 << (selectEffects - 1))))
        {
            continue;
        }

        // Check for school if specified
#if defined (TBC) || defined (WOTLK) || defined (CATA)
        if (uiSchool >= 0 && pTempSpell->SchoolMask & uiSchool)
        {
            continue;
        }
#elif defined(MISTS)
        if (uiSchool >= 0 && pTempSpell->GetSchoolMask() & uiSchool)
        {
            continue;
        }
#endif

        // Check for spell mechanic if specified
#if defined (CATA) || defined (MISTS)
        if (iMechanic >= 0 && pTempSpell->GetMechanic() != (uint32)iMechanic)
#else
        if (iMechanic >= 0 && pTempSpell->Mechanic != (uint32)iMechanic)
#endif
        {
            continue;
        }

        // Make sure that the spell uses the requested amount of power
#if defined (CATA) || defined (MISTS)
        if (uiPowerCostMin &&  pTempSpell->GetManaCost() < uiPowerCostMin)
#else
        if (uiPowerCostMin &&  pTempSpell->manaCost < uiPowerCostMin)
#endif
        {
            continue;
        }

#if defined (CATA) || defined (MISTS)
        if (uiPowerCostMax && pTempSpell->GetManaCost() > uiPowerCostMax)
#else
        if (uiPowerCostMax && pTempSpell->manaCost > uiPowerCostMax)
#endif
        {
            continue;
        }

        // Continue if we don't have the mana to actually cast this spell
#if defined (CATA)
        if (pTempSpell->GetManaCost() > m_creature->GetPower((Powers)pTempSpell->powerType))
#elif defined (MISTS)
        if (pTempSpell->GetManaCost() > m_creature->GetPower((Powers)pTempSpell->GetPowerType()))
#else
        if (pTempSpell->manaCost > m_creature->GetPower((Powers)pTempSpell->powerType))
#endif

        {
            continue;
        }

        // Get the Range
#if defined(MISTS)
        pTempRange = GetSpellRangeStore()->LookupEntry(pTempSpell->GetRangeIndex());
#else
        pTempRange = GetSpellRangeStore()->LookupEntry(pTempSpell->rangeIndex);
#endif
        // Spell has invalid range store so we can't use it
        if (!pTempRange)
        {
            continue;
        }

        // Check if the spell meets our range requirements
        if (fRangeMin && pTempRange->maxRange < fRangeMin)
        {
            continue;
        }

        if (fRangeMax && pTempRange->maxRange > fRangeMax)
        {
            continue;
        }

        // Check if our target is in range
        if (m_creature->IsWithinDistInMap(pTarget, pTempRange->minRange) || !m_creature->IsWithinDistInMap(pTarget, pTempRange->maxRange))
        {
            continue;
        }

        // All good so lets add it to the spell list
        apSpell[uiSpellCount] = pTempSpell;
        ++uiSpellCount;
    }

    // We got our usable spells so now lets randomly pick one
    if (!uiSpellCount)
    {
        return nullptr;
    }

    return apSpell[urand(0, uiSpellCount - 1)];
}

/**
 * @brief Checks if the creature can cast a specified spell on a target.
 * @param pTarget Pointer to the target.
 * @param pSpellEntry Pointer to the spell entry.
 * @param bTriggered Whether the spell is triggered.
 * @return True if the creature can cast the spell, false otherwise.
 */
bool ScriptedAI::CanCast(Unit* pTarget, SpellEntry const* pSpellEntry, bool bTriggered)
{
    // No target so we can't cast
    if (!pTarget || !pSpellEntry)
    {
        return false;
    }

    // Silenced so we can't cast
    if (!bTriggered && m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED))
    {
        return false;
    }

    // Check for power
#if defined (CATA)
    if (!bTriggered && m_creature->GetPower((Powers)pSpellEntry->powerType) < pSpellEntry->GetManaCost())
#elif defined (MISTS)
    if (!bTriggered && m_creature->GetPower((Powers)pSpellEntry->GetPowerType()) < pSpellEntry->GetManaCost())
#else
    if (!bTriggered && m_creature->GetPower((Powers)pSpellEntry->powerType) < pSpellEntry->manaCost)
#endif
    {
        return false;
    }

#if defined(MISTS)
    SpellRangeEntry const* pTempRange = GetSpellRangeStore()->LookupEntry(pSpellEntry->GetRangeIndex());
#else
    SpellRangeEntry const* pTempRange = GetSpellRangeStore()->LookupEntry(pSpellEntry->rangeIndex);
#endif

    // Spell has invalid range store so we can't use it
    if (!pTempRange)
    {
        return false;
    }

    // Unit is out of range of this spell
    if (!m_creature->IsInRange(pTarget, pTempRange->minRange, pTempRange->maxRange))
    {
        return false;
    }

    return true;
}

/**
 * @brief Fills the spell summary with information about spells.
 */
void FillSpellSummary()
{
    SpellSummary = new TSpellSummary[GetSpellStore()->GetNumRows()];

    SpellEntry const* pTempSpell;

    for (uint32 i = 0; i < GetSpellStore()->GetNumRows(); ++i)
    {
        SpellSummary[i].Effects = 0;
        SpellSummary[i].Targets = 0;

        pTempSpell = GetSpellStore()->LookupEntry(i);

        // This spell doesn't exist
        if (!pTempSpell)
        {
            continue;
        }

        for (uint8 j = 0; j < 3; ++j)
        {
#if defined (CATA) || defined (MISTS)
            SpellEffectEntry const* pSpellEffect = pTempSpell->GetSpellEffect(SpellEffectIndex(j));
            if (!pSpellEffect)
            {
                continue;
            }
#endif
            // Spell targets self
#if defined (CATA) || defined (MISTS)
            if (pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_SELF)
#else
            if (pTempSpell->EffectImplicitTargetA[j] == TARGET_SELF)
#endif
            {
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_SELF - 1);
            }

            // Spell targets a single enemy
#if defined (CATA) || defined (MISTS)
            if (pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_CHAIN_DAMAGE ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_CURRENT_ENEMY_COORDINATES)
#else
            if (pTempSpell->EffectImplicitTargetA[j] == TARGET_CHAIN_DAMAGE ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_CURRENT_ENEMY_COORDINATES)
#endif
            {
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_SINGLE_ENEMY - 1);
            }

            // Spell targets AoE at enemy
#if defined (CATA) || defined (MISTS)
            if (pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_ALL_ENEMY_IN_AREA ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_ALL_ENEMY_IN_AREA_INSTANT ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_CASTER_COORDINATES ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_ALL_ENEMY_IN_AREA_CHANNELED)
#else
            if (pTempSpell->EffectImplicitTargetA[j] == TARGET_ALL_ENEMY_IN_AREA ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_ALL_ENEMY_IN_AREA_INSTANT ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_CASTER_COORDINATES ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_ALL_ENEMY_IN_AREA_CHANNELED)
#endif
            {
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_AOE_ENEMY - 1);
            }

            // Spell targets an enemy
#if defined (CATA) || defined (MISTS)
            if (pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_CHAIN_DAMAGE ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_CURRENT_ENEMY_COORDINATES ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_ALL_ENEMY_IN_AREA ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_ALL_ENEMY_IN_AREA_INSTANT ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_CASTER_COORDINATES ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_ALL_ENEMY_IN_AREA_CHANNELED)
#else
            if (pTempSpell->EffectImplicitTargetA[j] == TARGET_CHAIN_DAMAGE ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_CURRENT_ENEMY_COORDINATES ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_ALL_ENEMY_IN_AREA ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_ALL_ENEMY_IN_AREA_INSTANT ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_CASTER_COORDINATES ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_ALL_ENEMY_IN_AREA_CHANNELED)
#endif
            {
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_ANY_ENEMY - 1);
            }

            // Spell targets a single friend(or self)
#if defined (CATA) || defined (MISTS)
            if (pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_SELF ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_SINGLE_FRIEND ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_SINGLE_PARTY)
#else
            if (pTempSpell->EffectImplicitTargetA[j] == TARGET_SELF ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_SINGLE_FRIEND ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_SINGLE_PARTY)
#endif
            {
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_SINGLE_FRIEND - 1);
            }

            // Spell targets aoe friends
#if defined (CATA) || defined (MISTS)
            if (pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_ALL_PARTY_AROUND_CASTER ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_AREAEFFECT_PARTY ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_CASTER_COORDINATES)
#else
            if (pTempSpell->EffectImplicitTargetA[j] == TARGET_ALL_PARTY_AROUND_CASTER ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_AREAEFFECT_PARTY ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_CASTER_COORDINATES)
#endif
            {
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_AOE_FRIEND - 1);
            }

            // Spell targets any friend(or self)
#if defined (CATA) || defined (MISTS)
            if (pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_SELF ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_SINGLE_FRIEND ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_SINGLE_PARTY ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_ALL_PARTY_AROUND_CASTER ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_AREAEFFECT_PARTY ||
                pTempSpell->GetEffectImplicitTargetAByIndex(SpellEffectIndex(j)) == TARGET_CASTER_COORDINATES)
#else
            if (pTempSpell->EffectImplicitTargetA[j] == TARGET_SELF ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_SINGLE_FRIEND ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_SINGLE_PARTY ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_ALL_PARTY_AROUND_CASTER ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_AREAEFFECT_PARTY ||
                pTempSpell->EffectImplicitTargetA[j] == TARGET_CASTER_COORDINATES)
#endif
            {
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_ANY_FRIEND - 1);
            }

            // Make sure that this spell includes a damage effect
#if defined (CATA) || defined (MISTS)
            if (pSpellEffect->Effect == SPELL_EFFECT_SCHOOL_DAMAGE ||
                pSpellEffect->Effect == SPELL_EFFECT_INSTAKILL ||
                pSpellEffect->Effect == SPELL_EFFECT_ENVIRONMENTAL_DAMAGE ||
                pSpellEffect->Effect == SPELL_EFFECT_HEALTH_LEECH)
#else
            if (pTempSpell->Effect[j] == SPELL_EFFECT_SCHOOL_DAMAGE ||
                pTempSpell->Effect[j] == SPELL_EFFECT_INSTAKILL ||
                pTempSpell->Effect[j] == SPELL_EFFECT_ENVIRONMENTAL_DAMAGE ||
                pTempSpell->Effect[j] == SPELL_EFFECT_HEALTH_LEECH)
#endif
            {
                SpellSummary[i].Effects |= 1 << (SELECT_EFFECT_DAMAGE - 1);
            }

            // Make sure that this spell includes a healing effect (or an apply aura with a periodic heal)
#if defined (CATA) || defined (MISTS)
            if (pSpellEffect->Effect == SPELL_EFFECT_HEAL ||
                    pSpellEffect->Effect == SPELL_EFFECT_HEAL_MAX_HEALTH ||
                    pSpellEffect->Effect == SPELL_EFFECT_HEAL_MECHANICAL ||
                    (pSpellEffect->Effect == SPELL_EFFECT_APPLY_AURA  && pTempSpell->GetEffectApplyAuraNameByIndex(SpellEffectIndex(j)) == 8))
#else
            if (pTempSpell->Effect[j] == SPELL_EFFECT_HEAL ||
                pTempSpell->Effect[j] == SPELL_EFFECT_HEAL_MAX_HEALTH ||
                pTempSpell->Effect[j] == SPELL_EFFECT_HEAL_MECHANICAL ||
                (pTempSpell->Effect[j] == SPELL_EFFECT_APPLY_AURA  && pTempSpell->EffectApplyAuraName[j] == 8))
#endif
            {
                SpellSummary[i].Effects |= 1 << (SELECT_EFFECT_HEALING - 1);
            }

            // Make sure that this spell applies an aura
#if defined (CATA) || defined (MISTS)
            if (pSpellEffect->Effect == SPELL_EFFECT_APPLY_AURA)
#else
            if (pTempSpell->Effect[j] == SPELL_EFFECT_APPLY_AURA)
#endif
            {
                SpellSummary[i].Effects |= 1 << (SELECT_EFFECT_AURA - 1);
            }
        }
    }
}

/**
 * @brief Resets the threat of the creature.
 */
void ScriptedAI::DoResetThreat()
{
    if (!m_creature->CanHaveThreatList() || m_creature->GetThreatManager().isThreatListEmpty())
    {
        script_error_log("DoResetThreat called for creature that either can not have threat list or has empty threat list (m_creature entry = %d)", m_creature->GetEntry());
        return;
    }

    ThreatList const& tList = m_creature->GetThreatManager().getThreatList();
    for (ThreatList::const_iterator itr = tList.begin(); itr != tList.end(); ++itr)
    {
        Unit* pUnit = m_creature->GetMap()->GetUnit((*itr)->getUnitGuid());

        if (pUnit && m_creature->GetThreatManager().getThreat(pUnit))
        {
            m_creature->GetThreatManager().modifyThreatPercent(pUnit, -100);
        }
    }
}

/**
 * @brief Teleports a player to a specified location.
 * @param pUnit Pointer to the unit to teleport.
 * @param fX X coordinate of the destination.
 * @param fY Y coordinate of the destination.
 * @param fZ Z coordinate of the destination.
 * @param fO Orientation at the destination.
 */
void ScriptedAI::DoTeleportPlayer(Unit* pUnit, float fX, float fY, float fZ, float fO)
{
    if (!pUnit)
    {
        return;
    }

    if (pUnit->GetTypeId() != TYPEID_PLAYER)
    {
        script_error_log("%s tried to teleport non-player (%s) to x: %f y:%f z: %f o: %f. Aborted.", m_creature->GetGuidStr().c_str(), pUnit->GetGuidStr().c_str(), fX, fY, fZ, fO);
        return;
    }

    ((Player*)pUnit)->TeleportTo(pUnit->GetMapId(), fX, fY, fZ, fO, TELE_TO_NOT_LEAVE_COMBAT);
}

/**
 * @brief Selects the friendly unit with the lowest HP within a specified range.
 * @param fRange Range to search for friendly units.
 * @param uiMinHPDiff Minimum HP difference to consider.
 * @return Pointer to the unit with the lowest HP, or nullptr if none found.
 */
Unit* ScriptedAI::DoSelectLowestHpFriendly(float fRange, uint32 uiMinHPDiff)
{
    Unit* pUnit = nullptr;

    MaNGOS::MostHPMissingInRangeCheck u_check(m_creature, fRange, uiMinHPDiff);
    MaNGOS::UnitLastSearcher<MaNGOS::MostHPMissingInRangeCheck> searcher(pUnit, u_check);

    Cell::VisitGridObjects(m_creature, searcher, fRange);

    return pUnit;
}

/**
 * @brief Finds friendly units that are crowd-controlled within a specified range.
 * @param fRange Range to search for friendly units.
 * @return List of friendly units that are crowd-controlled.
 */
std::list<Creature*> ScriptedAI::DoFindFriendlyCC(float fRange)
{
    std::list<Creature*> pList;

    MaNGOS::FriendlyCCedInRangeCheck u_check(m_creature, fRange);
    MaNGOS::CreatureListSearcher<MaNGOS::FriendlyCCedInRangeCheck> searcher(pList, u_check);

    Cell::VisitGridObjects(m_creature, searcher, fRange);

    return pList;
}

/**
 * @brief Finds friendly units missing a specific buff within a specified range.
 * @param fRange Range to search for friendly units.
 * @param uiSpellId ID of the buff spell.
 * @return List of friendly units missing the specified buff.
 */
std::list<Creature*> ScriptedAI::DoFindFriendlyMissingBuff(float fRange, uint32 uiSpellId)
{
    std::list<Creature*> pList;

    MaNGOS::FriendlyMissingBuffInRangeCheck u_check(m_creature, fRange, uiSpellId);
    MaNGOS::CreatureListSearcher<MaNGOS::FriendlyMissingBuffInRangeCheck> searcher(pList, u_check);

    Cell::VisitGridObjects(m_creature, searcher, fRange);

    return pList;
}

/**
 * @brief Gets a player at a minimum range from the creature.
 * @param fMinimumRange Minimum range to search for players.
 * @return Pointer to the player, or nullptr if none found.
 */
Player* ScriptedAI::GetPlayerAtMinimumRange(float fMinimumRange)
{
    Player* pPlayer = nullptr;

    MaNGOS::AnyPlayerInObjectRangeCheck check(m_creature, fMinimumRange);
    MaNGOS::PlayerSearcher<MaNGOS::AnyPlayerInObjectRangeCheck> searcher(pPlayer, check);

    Cell::VisitWorldObjects(m_creature, searcher, fMinimumRange);

    return pPlayer;
}

/**
 * @brief Sets the equipment slots for the creature.
 * @param bLoadDefault Whether to load the default equipment.
 * @param iMainHand ID of the main hand item.
 * @param iOffHand ID of the off hand item.
 * @param iRanged ID of the ranged item.
 */
void ScriptedAI::SetEquipmentSlots(bool bLoadDefault, int32 iMainHand, int32 iOffHand, int32 iRanged)
{
    if (bLoadDefault)
    {
        m_creature->LoadEquipment(m_creature->GetCreatureInfo()->EquipmentTemplateId, true);
        return;
    }

    if (iMainHand >= 0)
    {
        m_creature->SetVirtualItem(VIRTUAL_ITEM_SLOT_0, iMainHand);
    }

    if (iOffHand >= 0)
    {
        m_creature->SetVirtualItem(VIRTUAL_ITEM_SLOT_1, iOffHand);
    }

    if (iRanged >= 0)
    {
        m_creature->SetVirtualItem(VIRTUAL_ITEM_SLOT_2, iRanged);
    }
}

// Hacklike storage used for misc creatures that are expected to evade if outside of a certain area.
// It is assumed the information is found elsewhere and can be handled by mangos. So far no luck finding such information/way to extract it.
enum
{

#if defined (CLASSIC)
    NPC_BROODLORD               = 12017
#endif
#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
    NPC_BROODLORD               = 12017,
    NPC_VOID_REAVER             = 19516,
    NPC_JAN_ALAI                = 23578,
    NPC_SARTHARION              = 28860,
    NPC_TALON_KING_IKISS        = 18473,
    NPC_KARGATH_BLADEFIST       = 16808,
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    NPC_ANUBARAK                = 29120,
    NPC_SINDRAGOSA              = 36853,
    NPC_ZARITHRIAN              = 39746,
#endif
};

/**
 * @brief Checks if the creature should enter evade mode if it is out of the combat area.
 * @param uiDiff Time since the last update.
 * @return True if the creature should enter evade mode, false otherwise.
 */
bool ScriptedAI::EnterEvadeIfOutOfCombatArea(const uint32 uiDiff)
{
    if (m_uiEvadeCheckCooldown < uiDiff)
    {
        m_uiEvadeCheckCooldown = 2500;
    }
    else
    {
        m_uiEvadeCheckCooldown -= uiDiff;
        return false;
    }

    if (m_creature->IsInEvadeMode() || !m_creature->getVictim())
    {
        return false;
    }

    float fX = m_creature->GetPositionX();
    float fY = m_creature->GetPositionY();
    float fZ = m_creature->GetPositionZ();

    switch (m_creature->GetEntry())
    {
        case NPC_BROODLORD:  // broodlord (not move down stairs)
            if (fZ > 448.60f)
            {
                return false;
            }
            break;
#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
        case NPC_VOID_REAVER:  // void reaver (calculate from center of room)
            if (m_creature->GetDistance2d(432.59f, 371.93f) < 105.0f)
            {
                return false;
            }
            break;
        case NPC_JAN_ALAI:  // jan'alai (calculate by Z)
            if (fZ > 12.0f)
            {
                return false;
            }
            break;
        case NPC_SARTHARION:  // sartharion (calculate box)
            if (fX > 3218.86f && fX < 3275.69f && fY < 572.40f && fY > 484.68f)
            {
                return false;
            }
            break;
        case NPC_TALON_KING_IKISS:
        {
            float fX, fY, fZ;
            m_creature->GetRespawnCoord(fX, fY, fZ);
            if (m_creature->GetDistance2d(fX, fY) < 70.0f)
            {
                return false;
            }
            break;
        }
        case NPC_KARGATH_BLADEFIST:
            if (fX < 255.0f && fX > 205.0f)
            {
                return false;
            }
            break;

#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
        case NPC_ANUBARAK:
            if (fY < 281.0f && fY > 228.0f)
            {
                return false;
            }
            break;
        case NPC_SINDRAGOSA:
            if (fX > 4314.0f)
            {
                return false;
            }
            break;
        case NPC_ZARITHRIAN:
            if (fZ > 87.0f)
            {
                return false;
            }
            break;
#endif
        default:
            script_error_log("EnterEvadeIfOutOfCombatArea used for creature entry %u, but does not have any definition.", m_creature->GetEntry());
            return false;
    }

    EnterEvadeMode();
    return true;
}

/**
 * @brief Provides AI information for Scripted_NoMovementAI.
 * @param reader Reference to the ChatHandler.
 */
void Scripted_NoMovementAI::GetAIInformation(ChatHandler& reader)
{
    reader.PSendSysMessage("Subclass of Scripted_NoMovementAI");
}

/**
 * @brief Starts the creature's attack on a specified unit without movement.
 * @param pWho Pointer to the unit to attack.
 */
void Scripted_NoMovementAI::AttackStart(Unit* pWho)
{
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    if (!m_creature->CanInitiateAttack())
    {
        return;
    }
#endif
    if (pWho && m_creature->Attack(pWho, true))
    {
        m_creature->AddThreat(pWho);
        m_creature->SetInCombatWith(pWho);
        pWho->SetInCombatWith(m_creature);

        DoStartNoMovement(pWho);
    }
}
