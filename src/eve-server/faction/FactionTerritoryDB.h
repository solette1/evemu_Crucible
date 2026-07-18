/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2021 The EVEmu Team
    For the latest information visit https://evemu.dev
    ------------------------------------------------------------------------------------
*/

#ifndef __FACTION_TERRITORY_DB__H__INCL__
#define __FACTION_TERRITORY_DB__H__INCL__

#include "ServiceDB.h"

struct FactionTerritoryState {
    uint32 solarSystemID = 0;
    uint32 occupyingFactionID = 0;
    bool contested = false;
    float contestLevel = 0.0f;
    int64 lastUpdated = 0;
};

class FactionTerritoryDB : public ServiceDB
{
public:
    bool GetSystemStateForUpdate(uint32 systemID, FactionTerritoryState& state, bool& found);
    bool InsertInitialSystemState(uint32 systemID, uint32 occupyingFactionID);
    bool InsertInfluenceEvent(uint32 systemID, uint32 attackerFactionID, uint32 defenderFactionID, float pointsDelta, const char* reason);
    bool UpdateSystemState(const FactionTerritoryState& state);
    bool BeginTransaction();
    bool CommitTransaction();
    bool RollbackTransaction();
    bool GetSystemState(uint32 systemID, FactionTerritoryState& state, bool& found);
};

#endif /* __FACTION_TERRITORY_DB__H__INCL__ */
