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

#ifndef WDTFILE_H
#define WDTFILE_H

#include <string>
#include <mpq.h>
#include "wmo.h"
#include "adtfile.h"

enum TerrainFlags {
    TERRAIN_HAS_ADT = 0x01
};

struct SMAreaInfo     // -> CMapAreaTableEntry
{
    uint32_t flags;
    uint32_t asyncId;    // only set during runtime.
};

/**
 * @brief
 *
 */
class WDTFile
{
    public:
        /**
         * @brief
         *
         * @param file_name
         * @param file_name1
         */
        WDTFile(HANDLE handle, char* file_name, char* file_name1);
        /**
         * @brief
         *
         */
        ~WDTFile(void);
        /**
         * @brief
         *
         * @param map_id
         * @param mapID
         * @return bool
         */
        bool init(char* map_id, unsigned int mapID);

        bool hasTerrain(int x, int y);

        std::string* gWmoInstansName; /**< TODO */
        int gnWMO, nMaps; /**< TODO */

        /**
         * @brief
         *
         * @param x
         * @param z
         * @return ADTFile
         */
        ADTFile* GetMap(int x, int y);

    private:
        MPQFile WDT; /**< TODO */
        static const int MAP_TILE_SIZE = 64;
        SMAreaInfo* mapAreaInfo[MAP_TILE_SIZE * MAP_TILE_SIZE];
        std::string filename; /**< TODO */
};

#endif
