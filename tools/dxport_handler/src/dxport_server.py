import os
import config
import socket
import logging
from datetime import datetime
from queue import Queue
from threading import Thread


LOG = logging.getLogger(__name__)


_MSG_START = chr(0x2)
_PARAM_DELIM = chr(0x1e)
_MSG_END = chr(0x3)
_BUF_SIZE = 1024


_TRANSACTION_MAX = 1000  # Max # of statements per transaction


class StatDb(object):
    def __init__(self, conn):
        self.conn = conn
        self.create_tables()

    def create_tables(self):
        self.conn.lock()
        self.conn.execute("""
            CREATE TABLE IF NOT EXISTS mob_kill (
                player_name TEXT,
                mob_vnum INTEGER,
                mob_room INTEGER,
                timestamp INTEGER
            );
        """)
        self.conn.execute("""
            CREATE TABLE IF NOT EXISTS player_connect (
                player_name TEXT,
                ip TEXT,
                timestamp INTEGER
            );
        """)
        self.conn.unlock()

    def begin_transaction(self):
        self.conn.lock()
        self.conn.execute("BEGIN TRANSACTION;")

    def end_transaction(self):
        self.conn.execute("END TRANSACTION;")
        self.conn.unlock()

    def add_mob_kill(self, player_name, mob_vnum, mob_room, timestamp):
        self.conn.execute("""
            INSERT INTO mob_kill values (?, ?, ?, ?);
            """, (
                player_name,
                mob_vnum,
                mob_room,
                timestamp
            ))

    def add_player_connect(self, player_name, ip, timestamp):
        self.conn.execute("""
            INSERT INTO player_connect values (?, ?, ?);
            """, (
                player_name,
                ip,
                timestamp
            ))


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

            msg = b''
            try:
                while True:
                    data = connection.recv(_BUF_SIZE)
                    if data == '':
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
