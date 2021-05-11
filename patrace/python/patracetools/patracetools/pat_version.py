#!/usr/bin/env python

import argparse
import struct
from collections import namedtuple
import sys


@classmethod
def make(cls, file):
    data = file.read(cls.size())
    return cls._make(struct.unpack(cls.fmt, data))


@classmethod
def size(cls):
    return struct.calcsize(cls.fmt)

HeaderHead = namedtuple('HeaderHead', 'toNext magicNo version')
HeaderHead.fmt = 'iii'
HeaderHead.size = size
HeaderHead.make = make


def main():
    parser = argparse.ArgumentParser(
        description='Prints the file version of a .pat file',
    )
    parser.add_argument('trace',
                        help='The path to the trace file')
    args = parser.parse_args()

    with open(args.trace, "rb") as f:
        hh = HeaderHead.make(f)

    if hh.magicNo != 538058770:
        print("{f} is not a .pat file".format(f=args.trace), file=sys.stderr)
        return 1

    print(hh.version - 2)

if __name__ == "__main__":
    sys.exit(main())
