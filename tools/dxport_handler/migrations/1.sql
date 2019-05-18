CREATE TABLE IF NOT EXISTS mob_kill (
    player_name TEXT,
    mob_vnum INTEGER,
    mob_room INTEGER,
    timestamp INTEGER
);

CREATE TABLE IF NOT EXISTS player_connect (
    player_name TEXT,
    ip TEXT,
    timestamp INTEGER
);