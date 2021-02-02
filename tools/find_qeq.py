#!/usr/bin/env python3
import os
import sqlite3
import sys


QEQ_VNUMS = (
	10387,
	10389,
	10386,
	10380,
	10381,
	10382,
	10383,
	10384,
	10385,
	22050,
	10388,
	10398,
	22051,
	22052,
	22053,
	4700,
	4701,
	4702,
	4703,
	4704,
	4705,
	4706,
	4707,
	4708,
	4709,
	4710,
	4711,
	4712,
	4726,
	4900,
	4906,
)


def main():
	if len(sys.argv) != 2:
		print(
			("Usage:\n"
			 "  {} <player name>").format(sys.argv[0]))
		sys.exit(1)

	db_path = os.getenv("AEAEA_DB_PATH")
	if db_path is None or db_path == "":
		db_path = "/home/aarchon/aarchon-website/flask_app/instance/aeaea_db.sqlite3"

	conn = sqlite3.connect(db_path)

	c = conn.cursor()

	query = (
		"SELECT player_objs.owner, player_objs.player_name, player_objs.vnum, objs.name "
		+ "FROM player_objs "
		+ "LEFT JOIN objs on player_objs.vnum = objs.vnum "
		+ "WHERE player_objs.vnum in (" + ",".join('?' for _ in range(len(QEQ_VNUMS))) + ") "
		+ "AND player_objs.owner=?")

	params = QEQ_VNUMS + (sys.argv[1],)
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
		+ "WHERE box_objs.vnum in (" + ",".join('?' for _ in range(len(QEQ_VNUMS))) + ") "
		+ "AND box_objs.owner=?")
	params = QEQ_VNUMS + (sys.argv[1],)
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
