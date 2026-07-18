/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2021 The EVEmu Team
    For the latest information visit https://evemu.dev
    ------------------------------------------------------------------------------------
*/

#include "eve-server.h"

#include "faction/FactionTerritoryDB.h"

bool FactionTerritoryDB::GetSystemStateForUpdate(uint32 systemID, FactionTerritoryState& state, bool& found)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT solarSystemID, occupyingFactionID, contested, contestLevel, UNIX_TIMESTAMP(lastUpdated) "
        "FROM faction_territory_system WHERE solarSystemID = %u FOR UPDATE",
        systemID))
    {
        _log(DATABASE__ERROR, "Failed to lock territory state for system %u: %s", systemID, res.error.c_str());
        return false;
    }

    DBResultRow row;
    found = res.GetRow(row);
    if (!found)
        return true;

    state.solarSystemID = row.GetUInt(0);
    state.occupyingFactionID = row.GetUInt(1);
    state.contested = row.GetBool(2);
    state.contestLevel = row.GetFloat(3);
    state.lastUpdated = row.GetInt64(4);
    return true;
}

bool FactionTerritoryDB::InsertInitialSystemState(uint32 systemID, uint32 occupyingFactionID)
{
    DBerror err;
    return sDatabase.RunQuery(err,
        "INSERT INTO faction_territory_system (solarSystemID, occupyingFactionID, contested, contestLevel, lastUpdated) "
        "VALUES (%u, %u, 0, 0, CURRENT_TIMESTAMP) "
        "ON DUPLICATE KEY UPDATE occupyingFactionID = occupyingFactionID",
        systemID, occupyingFactionID);
}

bool FactionTerritoryDB::InsertInfluenceEvent(uint32 systemID, uint32 attackerFactionID, uint32 defenderFactionID, float pointsDelta, const char* reason)
{
    std::string escapedReason;
    sDatabase.DoEscapeString(escapedReason, reason == nullptr ? "unknown" : reason);

    DBerror err;
    return sDatabase.RunQuery(err,
        "INSERT INTO faction_territory_events (solarSystemID, attackerFactionID, defenderFactionID, pointsDelta, reason, createdAt) "
        "VALUES (%u, %u, %u, %.3f, '%s', CURRENT_TIMESTAMP)",
        systemID, attackerFactionID, defenderFactionID, pointsDelta, escapedReason.c_str());
}

bool FactionTerritoryDB::UpdateSystemState(const FactionTerritoryState& state)
{
    DBerror err;
    return sDatabase.RunQuery(err,
        "UPDATE faction_territory_system SET occupyingFactionID = %u, contested = %u, contestLevel = %.3f, lastUpdated = CURRENT_TIMESTAMP "
        "WHERE solarSystemID = %u",
        state.occupyingFactionID, state.contested ? 1 : 0, state.contestLevel, state.solarSystemID);
}

bool FactionTerritoryDB::BeginTransaction()
{
    DBerror err;
    return sDatabase.RunQuery(err, "START TRANSACTION");
}

bool FactionTerritoryDB::CommitTransaction()
{
    DBerror err;
    return sDatabase.RunQuery(err, "COMMIT");
}

bool FactionTerritoryDB::RollbackTransaction()
{
    DBerror err;
    return sDatabase.RunQuery(err, "ROLLBACK");
}

bool FactionTerritoryDB::IsRetryableError(const DBerror& err) const
{
    return (err.GetErrNo() == ER_LOCK_DEADLOCK) || (err.GetErrNo() == ER_LOCK_WAIT_TIMEOUT);
}

bool FactionTerritoryDB::GetSystemState(uint32 systemID, FactionTerritoryState& state, bool& found)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT solarSystemID, occupyingFactionID, contested, contestLevel, UNIX_TIMESTAMP(lastUpdated) "
        "FROM faction_territory_system WHERE solarSystemID = %u",
        systemID))
    {
        _log(DATABASE__ERROR, "Failed to query territory state for system %u: %s", systemID, res.error.c_str());
        return false;
    }

    DBResultRow row;
    found = res.GetRow(row);
    if (!found)
        return true;

    state.solarSystemID = row.GetUInt(0);
    state.occupyingFactionID = row.GetUInt(1);
    state.contested = row.GetBool(2);
    state.contestLevel = row.GetFloat(3);
    state.lastUpdated = row.GetInt64(4);
    return true;
}
