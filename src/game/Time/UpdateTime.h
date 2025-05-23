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

#ifndef UPDATETIME_H
#define UPDATETIME_H

#include "Common.h"
#include "Timer.h"

#include <array>
#include <string>

#define AVG_DIFF_COUNT 500

class UpdateTime
{
    using DiffTableArray = std::array<uint32, AVG_DIFF_COUNT>;

public:
    uint32 GetAverageUpdateTime() const;
    uint32 GetTimeWeightedAverageUpdateTime() const;
    uint32 GetMaxUpdateTime() const;
    uint32 GetMaxUpdatTimeOfCurrentTable() const;
    uint32 GetLastUpdateTime() const;

    void UpdateWithDiff(uint32 diff);

    void RecordUpdateTimeReset();

protected:
    UpdateTime();

    void _RecordUpdateTimeDuration(std::string const& text, uint32 minUpdateTime);

private:
    DiffTableArray _updateTimeDataTable;
    uint32 _averageUpdateTime;
    uint32 _totalUpdateTime;
    uint32 _updateTimeTableIndex;
    uint32 _maxUpdateTime;
    uint32 _maxUpdateTimeOfLastTable;
    uint32 _maxUpdateTimeOfCurrentTable;

    uint32 _recordedTime;
};

class WorldUpdateTime : public UpdateTime
{
public:
    WorldUpdateTime() : UpdateTime(), _recordUpdateTimeInverval(0), _recordUpdateTimeMin(0), _lastRecordTime(0) { }
    void LoadFromConfig();
    void SetRecordUpdateTimeInterval(uint32 t);
    void RecordUpdateTime(uint32 gameTimeMs, uint32 diff, uint32 sessionCount);
    void RecordUpdateTimeDuration(std::string const& text);

private:
    uint32 _recordUpdateTimeInverval;
    uint32 _recordUpdateTimeMin;
    uint32 _lastRecordTime;
};

extern WorldUpdateTime sWorldUpdateTime;

#endif
