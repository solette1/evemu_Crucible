-- Faction territory warfare state and event history
-- +migrate Up
CREATE TABLE IF NOT EXISTS faction_territory_system (
    solarSystemID INT(10) UNSIGNED NOT NULL,
    occupyingFactionID INT(10) UNSIGNED NOT NULL DEFAULT 0,
    contested TINYINT(1) NOT NULL DEFAULT 0,
    contestLevel FLOAT NOT NULL DEFAULT 0,
    lastUpdated TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (solarSystemID),
    KEY idx_faction_territory_contested (contested),
    KEY idx_faction_territory_updated (lastUpdated)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS faction_territory_events (
    eventID BIGINT(20) UNSIGNED NOT NULL AUTO_INCREMENT,
    solarSystemID INT(10) UNSIGNED NOT NULL,
    attackerFactionID INT(10) UNSIGNED NOT NULL,
    defenderFactionID INT(10) UNSIGNED NOT NULL,
    pointsDelta FLOAT NOT NULL,
    reason VARCHAR(64) NOT NULL,
    createdAt TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (eventID),
    KEY idx_faction_territory_events_system (solarSystemID),
    KEY idx_faction_territory_events_created (createdAt)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- +migrate Down
DROP TABLE IF EXISTS faction_territory_events;
DROP TABLE IF EXISTS faction_territory_system;
