#!/usr/bin/env python3.6

import os 
import time
import sqlite3
import requests


DB_NAME = "aeaea_discord.sqlite3"
WEBHOOK = "https://discord.com/api/webhooks/1125865004349014127/IFRrSwZP576D8x7FHLh4WNVvvkhTHZdVpCzIy_5Tmphpmg9hJ2iWsGQZyFnHWlJSDEm-"


def main():
    dir_path = os.path.dirname(os.path.realpath(__file__))
    db_path = os.path.realpath(os.path.join(dir_path, "..", "..", "data", "area", DB_NAME))
    print(db_path)
    con = sqlite3.connect(db_path)
    cur = con.cursor()

    while True:
        res = cur.execute("SELECT rowid, message FROM info_messages ORDER BY rowid LIMIT 1;");
        row = res.fetchone()
        if row is not None:
            try:
                data = {'content': f"`{row[1]}`"}
                requests.post(WEBHOOK, json=data)
            except Exception as ex:
                print(ex)

            res = cur.execute(f"DELETE FROM info_messages WHERE rowid = {row[0]};")
            con.commit()

        time.sleep(10)



if __name__ == "__main__":
    main()
