#!/usr/bin/env python3
"""Serialize EE compiler invocations sharing the legacy Windows include files.

SN assembly intermittently fails to open labels.inc when concurrent Wine
compiler invocations read it. Keep the lock through the driver/assembler exit;
GNU cross-assembly and other build work can still run in parallel.
"""

import os
from pathlib import Path
import subprocess
import sys


def main():
    if len(sys.argv) < 2:
        raise SystemExit('usage: run_ee_compiler.py COMMAND [ARG ...]')
    lock_path = Path(__file__).resolve().parent / 'cc' / '.compile.lock'
    with lock_path.open('a+b') as lock:
        if os.name == 'nt':
            import msvcrt
            import time
            if os.fstat(lock.fileno()).st_size == 0:
                lock.write(b'\0')
                lock.flush()
            while True:
                lock.seek(0)
                try:
                    msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
                    break
                except OSError:
                    time.sleep(0.1)
        else:
            import fcntl
            fcntl.flock(lock.fileno(), fcntl.LOCK_EX)
        return subprocess.run(sys.argv[1:]).returncode


if __name__ == '__main__':
    raise SystemExit(main())
