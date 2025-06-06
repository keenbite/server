/* 
 * Project: KeenCore
 * License: GNU General Public License v2.0 or later (GPL-2.0+)
 *
 * This file is part of KeenCore.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Originally based on MaNGOS (Massive Network Game Object Server)
 * Copyright (C) 2005-2025 MaNGOS project <https://getmangos.eu>
 */

/** \file
    \ingroup authserver
*/

#include "Common.h"
#include "RealmList.h"
#include "Auth/AuthCodes.h"
#include "Util.h"                                           // for Tokens typedef
#include "Policies/Singleton.h"
#include "Database/DatabaseEnv.h"

INSTANTIATE_SINGLETON_1(RealmList);

extern DatabaseType LoginDatabase;

static const RealmBuildInfo ExpectedAuthServerClientBuilds[] =
{
    // highest supported build, also auto accept all above for simplify future supported builds testing
    {40000, 9, 0, 0, ' '},  // SHADOWLANDS
    {35662, 8, 3, 7, ' '},  // BFA
    {26972, 7, 3, 5, ' '},  // Legion
    {21742, 6, 2, 4, ' '},  // WOD
    {18414, 5, 4, 8, ' '},  // MOP
    {18273, 5, 4, 8, ' '},  // MOP
    {15595, 4, 3, 4, ' '},  // CATA
    {12340, 3, 3, 5, 'a'},  // WOTLK
    {8606,  2, 4, 3, ' '},  // TBC
    {6141,  1, 12, 3, ' '}, // Vanilla - Chinese
    {6005,  1, 12, 2, ' '}, // Vanilla - Spanish
    {5875,  1, 12, 1, ' '}, // Vanilla
    {0,     0, 0, 0, ' '}                                   // terminator
};

RealmBuildInfo const* FindBuildInfo(uint16 _build)
{
    // first build is low bound of always accepted range
    if (_build >= ExpectedAuthServerClientBuilds[0].build)
    {
        return &ExpectedAuthServerClientBuilds[0];
    }

    // continue from 1 with explicit equal check
    for (int i = 1; ExpectedAuthServerClientBuilds[i].build; ++i)
        if (_build == ExpectedAuthServerClientBuilds[i].build)
        {
            return &ExpectedAuthServerClientBuilds[i];
        }

    // none appropriate build
    return NULL;
}

RealmList::RealmList() : m_UpdateInterval(0), m_NextUpdateTime(time(NULL))
{
}

RealmList& sRealmList
{
    static RealmList realmlist;
    return realmlist;
}

RealmVersion RealmList::BelongsToVersion(uint32 build) const
{
    RealmBuildVersionMap::const_iterator it;
    if ((it = m_buildToVersion.find(build)) != m_buildToVersion.end())
    {
        return it->second;
    }
    else
    {
        return REALM_VERSION_VANILLA;
    }
}

RealmList::RealmListIterators RealmList::GetIteratorsForBuild(uint32 build) const
{
    RealmVersion version = BelongsToVersion(build);
    if (version >= REALM_VERSION_COUNT)
    {
        return RealmListIterators(
            m_realmsByVersion[0].end(),
            m_realmsByVersion[0].end()
            );
    }
    return RealmListIterators(
        m_realmsByVersion[uint32(version)].begin(),
        m_realmsByVersion[uint32(version)].end()
        );

}

/// Load the realm list from the database
void RealmList::Initialize(uint32 updateInterval)
{
    m_UpdateInterval = updateInterval;

    InitBuildToVersion();

    ///- Get the content of the realmlist table in the database
    UpdateRealms(true);
}

uint32 RealmList::NumRealmsForBuild(uint32 build) const
{
    return m_realmsByVersion[BelongsToVersion(build)].size();
}

void RealmList::AddRealmToBuildList(const Realm& realm)
{
    RealmBuilds builds = realm.realmbuilds;
    int buildNumber = *(builds.begin());
    m_realmsByVersion[BelongsToVersion(buildNumber)].push_back(&realm);
}

void RealmList::InitBuildToVersion()
{
    m_buildToVersion[5875] = REALM_VERSION_VANILLA;
    m_buildToVersion[6005] = REALM_VERSION_VANILLA;
    m_buildToVersion[6141] = REALM_VERSION_VANILLA;

    m_buildToVersion[8606] = REALM_VERSION_TBC;

    m_buildToVersion[12340] = REALM_VERSION_WOTLK;

    m_buildToVersion[15595] = REALM_VERSION_CATA;

    m_buildToVersion[18273] = REALM_VERSION_MOP;
    m_buildToVersion[18414] = REALM_VERSION_MOP;

    m_buildToVersion[21742] = REALM_VERSION_WOD;

    m_buildToVersion[26972] = REALM_VERSION_LEGION;

    m_buildToVersion[35662] = REALM_VERSION_BFA;

    m_buildToVersion[40000] = REALM_VERSION_SHADOWLANDS;
}

void RealmList::UpdateRealm(uint32 ID, const std::string& name, ACE_INET_Addr const& address, ACE_INET_Addr const& localAddr, ACE_INET_Addr const& localSubmask, uint32 port, uint8 icon, RealmFlags realmflags, uint8 timezone, AccountTypes allowedSecurityLevel, float popu, const std::string& builds)
{
    ///- Create new if not exist or update existed
    Realm& realm = m_realms[name];

    realm.m_ID       = ID;
    realm.name       = name;
    realm.icon       = icon;
    realm.realmflags = realmflags;
    realm.timezone   = timezone;
    realm.allowedSecurityLevel = allowedSecurityLevel;
    realm.populationLevel      = popu;

    Tokens tokens = StrSplit(builds, " ");
    Tokens::iterator iter;

    for (iter = tokens.begin(); iter != tokens.end(); ++iter)
    {
        uint32 build = atol((*iter).c_str());
        realm.realmbuilds.insert(build);
    }

    uint16 first_build = !realm.realmbuilds.empty() ? *realm.realmbuilds.begin() : 0;

    if (first_build)
    {
        AddRealmToBuildList(realm);
    }
    else
    {
        sLog.outError("You don't seem to have added any allowed realmbuilds to the realm: %s"
                      " and therefore it will not be listed to anyone", name.c_str());
    }
    realm.realmBuildInfo.build = first_build;
    realm.realmBuildInfo.major_version = 0;
    realm.realmBuildInfo.minor_version = 0;
    realm.realmBuildInfo.bugfix_version = 0;
    realm.realmBuildInfo.hotfix_version = ' ';

    if (first_build)
        if (RealmBuildInfo const* bInfo = FindBuildInfo(first_build))
            if (bInfo->build == first_build)
            {
                realm.realmBuildInfo = *bInfo;
            }

    ///- Append port to IP address.
    realm.ExternalAddress = address;
    realm.LocalAddress = localAddr;
    realm.LocalSubnetMask = localSubmask;
}

void RealmList::UpdateIfNeed()
{
    // maybe disabled or updated recently
    if (!m_UpdateInterval || m_NextUpdateTime > time(NULL))
    {
        return;
    }

    m_NextUpdateTime = time(NULL) + m_UpdateInterval;

    // Clears Realm list
    m_realms.clear();
    for (int i = 0; i < REALM_VERSION_COUNT; ++i)
    {
        m_realmsByVersion[i].clear();
    }

    // Get the content of the realmlist table in the database
    UpdateRealms(false);
}

void RealmList::UpdateRealms(bool init)
{
    DETAIL_LOG("Updating Realm List...");

    ////                                               0     1       2          3               4                  5       6       7             8           9                       10            11
    QueryResult* result = LoginDatabase.Query("SELECT `id`, `name`, `address`, `localAddress`, `localSubnetMask`, `port`, `icon`, `realmflags`, `timezone`, `allowedSecurityLevel`, `population`, `realmbuilds` FROM `realmlist` WHERE (`realmflags` & 1) = 0 ORDER BY `name`");

    ///- Circle through results and add them to the realm map
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 Id                       = fields[0].GetUInt32();
            std::string name                = fields[1].GetString();
            std::string externalAddress     = fields[2].GetString();
            std::string localAddress        = fields[3].GetString();
            std::string localSubmask        = fields[4].GetString();
            uint32 port                     = fields[5].GetUInt32();
            uint8 icon                      = fields[6].GetUInt8();
            uint8 realmflags                = fields[7].GetUInt8();
            uint8 timezone                  = fields[8].GetUInt8();
            uint8 allowedSecurityLevel      = fields[9].GetUInt8();
            float population                = fields[10].GetFloat();
            std::string realmbuilds         = fields[11].GetString();

            ACE_INET_Addr externalAddr(port, externalAddress.c_str(), AF_INET);
            ACE_INET_Addr localAddr(port, localAddress.c_str(), AF_INET);
            ACE_INET_Addr submask(0, localSubmask.c_str(), AF_INET);

            if (realmflags & ~(REALM_FLAG_OFFLINE | REALM_FLAG_NEW_PLAYERS | REALM_FLAG_RECOMMENDED | REALM_FLAG_SPECIFYBUILD))
            {
                sLog.outError("Realm (id %u, name '%s') can only be flagged as OFFLINE (mask 0x02), NEWPLAYERS (mask 0x20), RECOMMENDED (mask 0x40), or SPECIFICBUILD (mask 0x04) in DB", Id, name.c_str());
                realmflags &= (REALM_FLAG_OFFLINE | REALM_FLAG_NEW_PLAYERS | REALM_FLAG_RECOMMENDED | REALM_FLAG_SPECIFYBUILD);
            }

            UpdateRealm(Id, name, externalAddr, localAddr, submask, port, icon, RealmFlags(realmflags), timezone, (allowedSecurityLevel <= SEC_ADMINISTRATOR ? AccountTypes(allowedSecurityLevel) : SEC_ADMINISTRATOR), population, realmbuilds);

            if (init)
            {
                sLog.outString("Added realm id %u, name '%s'",  Id, name.c_str());
            }
        }
        while (result->NextRow());
        delete result;
    }
}
