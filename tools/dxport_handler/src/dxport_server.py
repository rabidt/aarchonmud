import os
import time
import config
import socket
import logging
import sqlite3
from datetime import datetime
from queue import Queue, Empty
from threading import Thread, Lock


LOG = logging.getLogger(__name__)


_MSG_START = chr(0x2)
_PARAM_DELIM = chr(0x1e)
_MSG_END = chr(0x3)
_BUF_SIZE = 1024


_TRANSACTION_MAX = 1000  # Max # of statements per transaction


class StatDb(object):
    def __init__(self, db_path):
        self._db_path = db_path

        self._stop_request = False
        self._stop_complete = False
        self._q = Queue()
        self._lock = Lock()
        self._loop_thread = Thread(target=self._loop)
        self._loop_thread.setDaemon(False)
        self._loop_thread.start()

    def _loop(self):
        # have to open the connection in the same thread
        # we will use it in
        self._conn = sqlite3.connect(self._db_path)
        self._conn.isolation_level = None
        curs = self._conn.cursor()

        # Schema migrations
        while True:
            curr_ver = curs.execute("PRAGMA user_version").fetchone()[0]
            LOG.info("Schema version is {}".format(curr_ver))
            tgt_ver = curr_ver + 1
            tgt_path = "migrations/{}.sql".format(tgt_ver)
            if not os.path.exists(tgt_path):
                LOG.info("Schema at latest version")
                break
            else:
                curs.execute("BEGIN TRANSACTION")
                script = open(tgt_path, 'r').read()
                script += "\nPRAGMA user_version = {}".format(tgt_ver)
                curs.executescript(script)

        # TODO: fix logic to finish transactions before stop?
        while True:
            if self._stop_request is True:
                self._conn.close()
                self._stop_complete = True
                return

            try:
                args, kwargs = self._q.get(True, 1)
            except Empty:
                continue

            try:
                curs.execute(*args, **kwargs)
            except Exception as ex:
                LOG.exception("{},{}".format(args, kwargs))

    def stop(self):
        LOG.info("Stopping StatDb")
        self._stop_request = True
        while True:
            if self._stop_complete is True:
                LOG.info("StatDb stopped")
                return
            time.sleep(1)

    def execute(self, *args, **kwargs):
        self._q.put((args, kwargs))

    def begin_transaction(self):
        self._lock.acquire()
        self.execute("BEGIN TRANSACTION;")

    def end_transaction(self):
        self.execute("END TRANSACTION;")
        self._lock.release()

    def add_mob_kill(self, player_name, mob_vnum, mob_room, timestamp):
        self.execute("""
            INSERT INTO mob_kill values (?, ?, ?, ?);
            """, (
                player_name,
                mob_vnum,
                mob_room,
                timestamp
            ))

    def add_player_connect(self, player_name, ip, timestamp):
        self.execute("""
            INSERT INTO player_connect values (?, ?, ?);
            """, (
                player_name,
                ip,
                timestamp
            ))

    def add_quest_request(self, player_name, quest_id, player_level, is_hard, giver_vnum, obj_vnum, mob_vnum, room_vnum):
        self.execute("""
            INSERT INTO quests (
                player_name,
                request_time,
                player_level,
                is_hard,
                giver_vnum,
                obj_vnum,
                mob_vnum,
                room_vnum)
            VALUES (?,?,?,?,?,?,?,?)
            """, (
                player_name,
                quest_id,
                player_level,
                is_hard,
                giver_vnum,
                None if obj_vnum == '0' else obj_vnum,
                None if mob_vnum == '0' else mob_vnum,
                None if room_vnum == '0' else room_vnum))

    def add_quest_complete(self, player_name, quest_id, end_time, completer_vnum, silver, qp, prac, exp):
        self.execute("""
            UPDATE quests
            SET end_time = ?,
                end_type = 'complete',
                completer_vnum = ?,
                silver = ?,
                qp = ?,
                prac = ?,
                exp = ?
            WHERE player_name = ? AND request_time = ?
            """, (end_time, completer_vnum, silver, qp, prac, exp, player_name, quest_id))

    def add_quest_timeout(self, player_name, quest_id, end_time):
        self.execute("""
            UPDATE quests
            SET end_time = ?,
                end_type = 'timeout'
            WHERE player_name = ? and request_time = ?
            """, (end_time, player_name, quest_id))

    def add_quest_giveup(self, player_name, quest_id, end_time):
        self.execute("""
            UPDATE quests
            SET end_time = ?,
                end_type = 'giveup'
            WHERE player_name = ? and request_time = ?
            """, (end_time, player_name, quest_id))


class MessageHandler(object):
    def __init__(self, stat_db, reload_handler):
        self.stat_db = stat_db
        self.reload_handler = reload_handler
        self.msg_queue = Queue()
        self.hndl_loop_thread = Thread(target=self._hndl_loop)
        self.hndl_loop_thread.setDaemon(True)
        self.hndl_loop_thread.start()

    def _hndl_loop(self):
        count = 0

        while True:
            msg = self.msg_queue.get()
            params = msg.split(_PARAM_DELIM)
            name = params[0]

            if not hasattr(self, name):
                LOG.error('No handler for {}'.format(name))
                continue

            if count == 0:
                self.stat_db.begin_transaction()

            count += 1
            method = getattr(self, name)
            method(*(params[1:]))

            if self.msg_queue.empty() or count >= _TRANSACTION_MAX:
                self.stat_db.end_transaction()
                count = 0

    def handle_message(self, msg):
        self.msg_queue.put(msg)

    def mob_kill(self, player_name, mob_vnum, mob_room, timestamp):
        LOG.debug("{} killed vnum {} at {}".format(
            player_name,
            mob_vnum,
            datetime.fromtimestamp(float(timestamp))))
        self.stat_db.add_mob_kill(player_name, mob_vnum, mob_room, timestamp)

    def player_connect(self, player_name, ip, timestamp):
        LOG.debug("{} connected from ip {} at {}".format(
            player_name,
            ip,
            datetime.fromtimestamp(float(timestamp))))
        self.stat_db.add_player_connect(player_name, ip, timestamp)

    def quest_request(self, player_name, quest_id, player_level, is_hard, giver_vnum, obj_vnum, mob_vnum, room_vnum):
        LOG.debug("{} quest_request, hard: {}".format(player_name, is_hard))
        self.stat_db.add_quest_request(player_name, quest_id, player_level, is_hard, giver_vnum, obj_vnum, mob_vnum, room_vnum)

    def quest_complete(self, player_name, quest_id, complete_time, completer_vnum, silver, qp, prac, exp):
        LOG.debug("{} quest_complete, qp: {}".format(player_name, qp))
        self.stat_db.add_quest_complete(player_name, quest_id, complete_time, completer_vnum, silver, qp, prac, exp)

    def quest_timeout(self, player_name, quest_id, end_time):
        self.stat_db.add_quest_timeout(player_name, quest_id, end_time)

    def quest_giveup(self, player_name, quest_id, end_time):
        self.stat_db.add_quest_giveup(player_name, quest_id, end_time)

    def reload(self):
        self.reload_handler()


class Server(object):
    def __init__(self, msg_hndlr):
        self.message_handler = msg_hndlr

    def handle_message(self, msg):
        self.message_handler.handle_message(msg)

    def start_server(self):
        # Make sure the socket does not already exist
        try:
            os.unlink(config.SOCK_PATH)
        except OSError:
            if os.path.exists(config.SOCK_PATH):
                raise

        if 'DXPORT_sockfd' in os.environ:
            # reload case
            fd = int(os.environ['DXPORT_sockfd'])
            LOG.info('Server socket fd is %d', fd)
            sock = socket.socket(fileno=fd)
        else:
            # Create a UDS socket
            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

            # Bind the socket to the port
            LOG.info("Starting up on {}".format(config.SOCK_PATH))
            sock.bind(config.SOCK_PATH)

            # Listen for incoming connections
            sock.listen(1)

            os.environ['DXPORT_sockfd'] = str(sock.fileno())
        
        sock.set_inheritable(True)

        while True:
            # reload case
            if 'DXPORT_connfd' in os.environ:
                fd = int(os.environ['DXPORT_connfd'])
                LOG.info('Conn socket fd is %d', fd)
                connection = socket.socket(fileno=fd)
            else:
                # Wait for a connection
                LOG.info("Waiting for a connection.")
                # sock.settimeout(5)
                connection, client_address = sock.accept()

                LOG.info("Connection from mud established")
                os.environ['DXPORT_connfd'] = str(connection.fileno())

            connection.set_inheritable(True)

            msg = ''
            try:
                while True:
                    data = connection.recv(_BUF_SIZE)
                    if data == b'':
                        break

                    for v in data:
                        c = chr(v)
                        if c == _MSG_START:
                            msg = ''
                        elif c == _MSG_END:
                            self.handle_message(msg)
                            msg = ''
                        else:
                            msg += c

            except Exception as ex:
                LOG.exception("Unexpected Exception reading from mud.")
            finally:
                # Clean up the connection
                LOG.info("Connection closing.")
                connection.close()
                del os.environ['DXPORT_connfd']
