# Faction Territory Warfare (Crucible)

## Data model

Two tables are added by migration `sql/migrations/20260718172500-faction_territory.sql`:

- `faction_territory_system`
  - `solarSystemID` (PK)
  - `occupyingFactionID`
  - `contested` (`0/1`)
  - `contestLevel` (`0..100`)
  - `lastUpdated`
- `faction_territory_events`
  - `eventID` (PK)
  - `solarSystemID`
  - `attackerFactionID`
  - `defenderFactionID`
  - `pointsDelta`
  - `reason`
  - `createdAt`

Indexes are provided for system lookups, contested scans, and event time-range queries.

## Contest rules

Contest eligibility is centralized in `FactionTerritoryMgr::IsSystemContestable(systemID)`:

- security `<= 0.5` is contestable
- security `>= 0.6` is not contestable

For non-contestable systems, influence is denied and state is forced to uncontested (`contested = false`, `contestLevel = 0`).

## Update and flip logic

`FactionTerritoryMgr::ApplyFactionInfluence(...)`:

1. Starts a DB transaction.
2. Locks system state row with `SELECT ... FOR UPDATE`.
3. Persists every accepted influence mutation to `faction_territory_events`.
4. Clamps contest level to `[0, 100]`.
5. Flips occupying faction when threshold is reached.
6. Writes territory row update atomically in the same transaction.

Default flip behavior:

- If attacker reaches configured threshold (`FlipThreshold`, default `100`), system flips to attacker.
- Contest level resets to `0` on flip.

## Tick/reconciliation behavior

`FactionTerritoryMgr::Process()` runs from server tick flow and calls `ReconcileAndDecay()` at configured interval (`TickIntervalSeconds`).

`ReconcileAndDecay()`:

- scans contested systems,
- applies decay (`DecayRate`) toward zero,
- enforces security-band eligibility,
- writes updates and refreshes in-memory cache.

## Integration points

Current concrete source:

- `ShipSE::Killed` in `src/eve-server/system/Damage.cpp` adds influence for faction-vs-faction kills using server-side values only.

Extension point:

- TODO marker in kill hook indicates where FW objective/site completion events should call `ApplyFactionInfluence(...)`.

## Config

In `eve-server.xml` under `<factionTerritory>`:

- `TickIntervalSeconds`
- `DecayRate`
- `FlipThreshold`
- `PointsKill`
- `PointsSiteCompletion`

These map to `sConfig.factionTerritory` in `EVEServerConfig`.

## Debug visibility

`facWarMgr.GetTerritoryState(solarSystemID)` returns a lightweight keyval state:

- `solarSystemID`
- `occupyingFactionID`
- `contested`
- `contestLevel`
