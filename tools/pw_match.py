#!/usr/bin/env python3
""" Given a password, print out all the characters that have that password. """
import os
import sqlite3
import crypt
import sys


def main():
    if len(sys.argv) != 2:
        print(
            ("Usage:\n"
             "    {} <password>").format(sys.argv[0]))
        sys.exit(1)

    entered_pw = sys.argv[1]

    db_path = os.getenv("AEAEA_DB_PATH")
    if db_path is None or db_path == "":
        db_path = "/home/m256ada/website_proj/flask_app/instance/aeaea_db.sqlite3"
    

    conn = sqlite3.connect(db_path)

    c = conn.cursor()

    query = "SELECT name, pwd FROM players;"
    c.execute(query)
    rows = c.fetchall()

    for row in rows:
        hashed = crypt.crypt(entered_pw, row[0])
        if hashed == row[1]:
            print(row[0])
        

if __name__ == "__main__":
    main()
