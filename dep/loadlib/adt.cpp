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

#include "adt.h"

// Helper
int holetab_h[4] = {0x1111, 0x2222, 0x4444, 0x8888};
int holetab_v[4] = {0x000F, 0x00F0, 0x0F00, 0xF000};

bool isHole(int holes, int i, int j)
{
    int testi = i / 2;
    int testj = j / 4;
    if (testi > 3)
    {
        testi = 3;
    }
    if (testj > 3)
    {
        testj = 3;
    }
    return (holes & holetab_h[testi] & holetab_v[testj]) != 0;
}

//
// Adt file loader class
//
ADT_file::ADT_file()
{
    a_grid = 0;
}

ADT_file::~ADT_file()
{
    free();
}

void ADT_file::free()
{
    a_grid = 0;
    FileLoader::free();
}

//
// Adt file check function
//
bool ADT_file::prepareLoadedData()
{
    // Check parent
    if (!FileLoader::prepareLoadedData())
    {
        return false;
    }

    // Check and prepare MHDR
    a_grid = (adt_MHDR*)(GetData() + 8 + version->size);
    if (!a_grid->prepareLoadedData())
    {
        return false;
    }

    // funny offsets calculations because there is no mapping for them and they have variable lengths
    uint8* ptr = (uint8*)a_grid + a_grid->size + 8;
    uint32 mcnk_count = 0;
    memset(cells, 0, ADT_CELLS_PER_GRID * ADT_CELLS_PER_GRID * sizeof(adt_MCNK*));
    while (ptr < GetData() + GetDataSize())
    {
        uint32 header = *(uint32*)ptr;
        uint32 size = *(uint32*)(ptr + 4);
        if (header == 'MCNK')
        {
            cells[mcnk_count / ADT_CELLS_PER_GRID][mcnk_count % ADT_CELLS_PER_GRID] = (adt_MCNK*)ptr;
            ++mcnk_count;
        }

        // move to next chunk
        ptr += size + 8;
    }

    if (mcnk_count != ADT_CELLS_PER_GRID * ADT_CELLS_PER_GRID)
    {
        return false;
    }

    return true;
}

bool adt_MHDR::prepareLoadedData()
{
    if (fcc != 'MHDR')
    {
        return false;
    }

    if (size != sizeof(adt_MHDR) - 8)
    {
        return false;
    }

    // Check and prepare MCIN
    if (offsMCIN && !getMCIN()->prepareLoadedData())
    {
        return false;
    }

    // Check and prepare MH2O
    if (offsMH2O && !getMH2O()->prepareLoadedData())
    {
        return false;
    }

    return true;
}

bool adt_MCIN::prepareLoadedData()
{
    if (fcc != 'MCIN')
    {
        return false;
    }

    // Check cells data
    for (int i = 0; i < ADT_CELLS_PER_GRID; i++)
    {
        for (int j = 0; j < ADT_CELLS_PER_GRID; j++)
        {
            if (cells[i][j].offsMCNK && !getMCNK(i, j)->prepareLoadedData())
            {
                return false;
            }
        }
    }
    return true;
}

bool adt_MH2O::prepareLoadedData()
{
    if (fcc != 'MH2O')
    {
        return false;
    }

    // Check liquid data
//    for (int i=0; i<ADT_CELLS_PER_GRID;i++)
//        for (int j=0; j<ADT_CELLS_PER_GRID;j++)

    return true;
}

bool adt_MCNK::prepareLoadedData()
{
    if (fcc != 'MCNK')
    {
        return false;
    }

    // Check height map
    if (offsMCVT && !getMCVT()->prepareLoadedData())
    {
        return false;
    }

    // Check liquid data
    if (offsMCLQ && !getMCLQ()->prepareLoadedData())
    {
        return false;
    }

    return true;
}

bool adt_MCVT::prepareLoadedData()
{
    if (fcc != 'MCVT')
    {
        return false;
    }

    if (size != sizeof(adt_MCVT) - 8)
    {
        return false;
    }

    return true;
}

bool adt_MCLQ::prepareLoadedData()
{
    if (fcc != 'MCLQ')
    {
        return false;
    }

    return true;
}