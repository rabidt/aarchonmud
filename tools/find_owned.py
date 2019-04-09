#!/usr/bin/env python3
import os
import sqlite3
import sys


def main():
	if len(sys.argv) != 2:
		print(
			("Usage:\n"
			 "  {} <player name>").format(sys.argv[0]))
		sys.exit(1)

	db_path = os.getenv("AEAEA_DB_PATH")
	if db_path is None or db_path == "":
		db_path = "/home/m256ada/website_support/data/aeaea_db.sqlite3"

	conn = sqlite3.connect(db_path)

	c = conn.cursor()

	query = (
		"SELECT player_objs.owner, player_objs.player_name, player_objs.vnum, objs.name "
		+ "FROM player_objs "
		+ "LEFT JOIN objs on player_objs.vnum = objs.vnum "
		+ "WHERE player_objs.owner=?")

	params = (sys.argv[1],)
	print("Held by players:")
	print("{:20} | {:20} | {:8} | {:30}".format(
		"Owner",
		"Holder",
		"Vnum",
		"Name"))
	for row in c.execute(query, params):
		print("{:20} | {:20} | {:8} | {:30}".format(*row))

	print("")

	query = (
		"SELECT box_objs.owner, box_objs.player_name, box_objs.box_num, box_objs.vnum, objs.name "
		+ "FROM box_objs "
		+ "LEFT JOIN objs on box_objs.vnum = objs.vnum "
		+ "WHERE box_objs.owner=?")
	params = (sys.argv[1],)
	print("Held in boxes:")
	print("{:20} | {:20} | {:7} | {:8} | {:30}".format(
		"Owner",
		"Box Owner",
		"Box Num",
		"Vnum",
		"Name"))
	for row in c.execute(query, params):
		print("{:20} | {:20} | {:7} | {:8} | {:30}".format(*row))


if __name__ == '__main__':
	main()
