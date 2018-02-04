#!/usr/bin/env python3

import os
import signal


def main():
    pid = os.fork()
    if pid == 0:
        while True:
            signal.pause()
    os.kill(pid, signal.SIGKILL)
    while True:
        signal.pause()


if __name__ == '__main__':
    main()
