import time
import sqlite3
import logging
from threading import Thread, Lock
from Queue import Queue, Empty


LOG = logging.getLogger(__name__)


class DbConn(object):
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
        self._curs = self._conn.cursor()

        # TODO: fix logic to finish transactions before stop?
        while True:
            if self._stop_request is True:
                self._stop_complete = True
                return

            try:
                args, kwargs = self._q.get(True, 1)
            except Empty:
                continue
            # print args, kwargs
            try:
                self._curs.execute(*args, **kwargs)
            except Exception as ex:
                LOG.exception("{},{}".format(args, kwargs))
            
    def stop(self):
        LOG.info("Stopping DbConn")
        self._stop_request = True
        while True:
            if self._stop_complete is True:
                LOG.info("DbConn stopped")
                return
            time.sleep(1)

    def execute(self, *args, **kwargs):
        self._q.put((args, kwargs))

    def lock(self):
        self._lock.acquire()

    def unlock(self):
        self._lock.release()

