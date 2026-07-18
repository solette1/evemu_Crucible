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

#include "faction/FactionTerritoryMgr.h"

FactionTerritoryMgr::FactionTerritoryMgr()
: m_tickTimer(0, true)
{
    m_cache.clear();
}

int FactionTerritoryMgr::Initialize()
{
    m_tickTimer.Start(static_cast<uint32>(sConfig.factionTerritory.TickIntervalSeconds) * 1000);
    sLog.Blue("       FacTerrMgr", "Faction Territory Manager Initialized.");
    return 1;
}

void FactionTerritoryMgr::Close()
{
    m_cache.clear();
    sLog.Warning("       FacTerrMgr", "Faction Territory Manager has been closed.");
}

void FactionTerritoryMgr::Process()
{
    if (m_tickTimer.Check())
        ReconcileAndDecay();
}

float FactionTerritoryMgr::ClampContestLevel(float value)
{
    if (value < 0.0f)
        return 0.0f;
    if (value > 100.0f)
        return 100.0f;
    return value;
}

bool FactionTerritoryMgr::IsSystemContestable(uint32 systemID)
{
    SystemData data = SystemData();
    if (!sDataMgr.GetSystemData(systemID, data))
        return false;

    return (data.securityRating <= 0.5f);
}

FactionTerritoryState FactionTerritoryMgr::GetSystemTerritoryState(uint32 systemID)
{
    auto cacheItr = m_cache.find(systemID);
    if (cacheItr != m_cache.end())
        return cacheItr->second;

    FactionTerritoryState state;
    bool found = false;
    if (m_db.GetSystemState(systemID, state, found) && found) {
        m_cache[systemID] = state;
        return state;
    }

    SystemData data = SystemData();
    if (sDataMgr.GetSystemData(systemID, data)) {
        state.solarSystemID = systemID;
        state.occupyingFactionID = data.factionID;
        state.contested = false;
        state.contestLevel = 0.0f;
        m_cache[systemID] = state;
    }

    return state;
}

bool FactionTerritoryMgr::ApplyFactionInfluence(uint32 systemID, uint32 attackerFactionID, uint32 defenderFactionID, float pointsDelta, const char* reason)
{
    if ((systemID == 0) || (attackerFactionID == 0) || (attackerFactionID == defenderFactionID))
        return false;

    for (uint8 attempt = 0; attempt < 3; ++attempt) {
        if (!m_db.BeginTransaction())
            continue;

        FactionTerritoryState state;
        bool found = false;
        if (!m_db.GetSystemStateForUpdate(systemID, state, found)) {
            m_db.RollbackTransaction();
            continue;
        }

        if (!found) {
            SystemData data = SystemData();
            if (!sDataMgr.GetSystemData(systemID, data)) {
                m_db.RollbackTransaction();
                return false;
            }

            if (!m_db.InsertInitialSystemState(systemID, data.factionID)) {
                m_db.RollbackTransaction();
                continue;
            }

            if (!m_db.GetSystemStateForUpdate(systemID, state, found) || !found) {
                m_db.RollbackTransaction();
                continue;
            }
        }

        state.solarSystemID = systemID;

        if (!IsSystemContestable(systemID)) {
            state.contested = false;
            state.contestLevel = 0.0f;
            if (!m_db.UpdateSystemState(state)) {
                m_db.RollbackTransaction();
                continue;
            }
            if (!m_db.CommitTransaction()) {
                m_db.RollbackTransaction();
                continue;
            }
            m_cache[systemID] = state;
            return false;
        }

        float adjustedDelta = pointsDelta;
        if (state.occupyingFactionID == attackerFactionID)
            adjustedDelta = -std::fabs(pointsDelta);

        float updatedLevel = ClampContestLevel(state.contestLevel + adjustedDelta);
        bool flipped = false;

        if ((state.occupyingFactionID != attackerFactionID) &&
            (updatedLevel >= sConfig.factionTerritory.FlipThreshold)) {
            state.occupyingFactionID = attackerFactionID;
            state.contestLevel = 0.0f;
            state.contested = false;
            flipped = true;
        } else {
            state.contestLevel = updatedLevel;
            state.contested = (updatedLevel > 0.0f);
        }

        if (!m_db.InsertInfluenceEvent(systemID, attackerFactionID, defenderFactionID, adjustedDelta, reason)) {
            m_db.RollbackTransaction();
            continue;
        }

        if (!m_db.UpdateSystemState(state)) {
            m_db.RollbackTransaction();
            continue;
        }

        if (!m_db.CommitTransaction()) {
            m_db.RollbackTransaction();
            continue;
        }

        m_cache[systemID] = state;

        if (flipped) {
            _log(FACWAR__INFO, "Territory flip in system %u: new occupier faction %u", systemID, attackerFactionID);
        } else {
            _log(FACWAR__INFO, "Territory update in system %u: occupier=%u contested=%u level=%.2f delta=%.2f",
                systemID,
                state.occupyingFactionID,
                state.contested ? 1 : 0,
                state.contestLevel,
                adjustedDelta);
        }
        return true;
    }

    _log(FACWAR__WARNING, "Failed to apply faction influence in system %u after retries.", systemID);
    return false;
}

void FactionTerritoryMgr::ReconcileAndDecay()
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT solarSystemID, occupyingFactionID, contested, contestLevel, UNIX_TIMESTAMP(lastUpdated) "
        "FROM faction_territory_system WHERE contested = 1"))
    {
        _log(DATABASE__ERROR, "Failed reconciling faction territory contested systems: %s", res.error.c_str());
        return;
    }

    DBResultRow row;
    while (res.GetRow(row)) {
        FactionTerritoryState state;
        state.solarSystemID = row.GetUInt(0);
        state.occupyingFactionID = row.GetUInt(1);
        state.contested = row.GetBool(2);
        state.contestLevel = row.GetFloat(3);
        state.lastUpdated = row.GetInt64(4);

        if (!IsSystemContestable(state.solarSystemID)) {
            state.contested = false;
            state.contestLevel = 0.0f;
        } else {
            state.contestLevel = ClampContestLevel(state.contestLevel - sConfig.factionTerritory.DecayRate);
            state.contested = (state.contestLevel > 0.0f);
        }

        if (!m_db.UpdateSystemState(state))
            continue;

        m_cache[state.solarSystemID] = state;
    }
}
