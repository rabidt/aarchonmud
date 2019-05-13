import os
import sys
import signal
import config
import logging
from db_conn import DbConn
from dxport_server import Server, MessageHandler, StatDb


LOG = logging.getLogger()


def main():
    logging.basicConfig(stream=sys.stdout, level=getattr(logging, config.LOG_LEVEL))
    LOG.debug("main running.")

    conn = DbConn(config.DB_PATH)
    stat_db = StatDb(conn)

    def handle_reload():
        LOG.info("Handling reload request")
        conn.stop()
        os.execv(sys.executable, [sys.executable] + sys.argv)
        LOG.error("execv failed")

    msg_hndlr = MessageHandler(stat_db, handle_reload)
    server = Server(msg_hndlr)

    def handle_signals(signal, frame):
        LOG.info("Handling signal {}".format(signal))
        conn.stop()
        sys.exit(0)

    signal.signal(signal.SIGINT, handle_signals)
    signal.signal(signal.SIGTERM, handle_signals)

    try:
        server.start_server()
    except:
        conn.stop()
        raise


if __name__ == '__main__':
    main()
