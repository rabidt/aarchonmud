CREATE TABLE IF NOT EXISTS quests (
    player_name TEXT NOT NULL,
    request_time INTEGER NOT NULL,
    end_time INTEGER,
    end_type TEXT,
    giver_vnum INTEGER,
    completer_vnum INTEGER,
    is_hard INTEGER,
    obj_vnum INTEGER,
    mob_vnum INTEGER,
    room_vnum INTEGER,
    silver INTEGER,
    qp INTEGER,
    prac INTEGER,
    exp INTEGER,
    PRIMARY KEY(player_name, request_time)
);