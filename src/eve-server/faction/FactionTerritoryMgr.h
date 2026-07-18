/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2021 The EVEmu Team
    For the latest information visit https://evemu.dev
    ------------------------------------------------------------------------------------
*/

#ifndef __FACTION_TERRITORY_MGR__H__INCL__
#define __FACTION_TERRITORY_MGR__H__INCL__

#include "eve-server.h"

#include "faction/FactionTerritoryDB.h"

class FactionTerritoryMgr : public Singleton<FactionTerritoryMgr>
{
public:
    FactionTerritoryMgr();

    int Initialize();
    void Close();
    void Process();

    bool ApplyFactionInfluence(uint32 systemID, uint32 attackerFactionID, uint32 defenderFactionID, float pointsDelta, const char* reason);
    bool IsSystemContestable(uint32 systemID);
    FactionTerritoryState GetSystemTerritoryState(uint32 systemID);
    void ReconcileAndDecay();

private:
    static float ClampContestLevel(float value);

    FactionTerritoryDB m_db;
    Timer m_tickTimer;
    std::map<uint32, FactionTerritoryState> m_cache;
};

#define sFactionTerritoryMgr \
    (FactionTerritoryMgr::get())

#endif /* __FACTION_TERRITORY_MGR__H__INCL__ */
