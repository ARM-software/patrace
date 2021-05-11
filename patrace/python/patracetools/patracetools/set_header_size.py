#!/usr/bin/env python

import argparse
import sys
import struct
from collections import namedtuple


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


HeaderV3 = namedtuple('HeaderV3', 'toNext magicNo version jsonLength jsonFileBegin jsonFileEnd')
HeaderV3.fmt = 'iiiill'
HeaderV3.size = size
HeaderV3.make = make


def main():
    parser = argparse.ArgumentParser(
        description='Adjusts the size of the header of a .pat trace',
    )
    parser.add_argument('trace',
                        help='The path to the trace file')
    parser.add_argument('size', type=int,
                        help='The number of bytes that will be reserved for the json data in the header.')

    args = parser.parse_args()

    with open(args.trace, "r+b") as f:
        hh = HeaderHead.make(f)

        if hh.version < 5:
            print("This script only support header format V3 and above", file=sys.stderr)
            return 1

        f.seek(0)
        header = HeaderV3.make(f)

        f.seek(header.jsonFileBegin)

        original_reservered_json_size = header.jsonFileEnd - header.jsonFileBegin

        if args.size < header.jsonLength:
            print("New header size must larger than the existing json data.", file=sys.stderr)
            return 1

        diff = args.size - original_reservered_json_size

        if diff > 0:
            print("Expanding header section with {size} bytes".format(size=diff))

            # Move the data section
            padding = b'\0' * diff
            f.seek(header.jsonFileEnd)
            chunk0 = padding

            while True:
                chunk1 = f.read(diff)
                s = len(chunk1)
                f.seek(-s, 1)
                f.write(chunk0)
                f.flush()

                chunk0 = chunk1

                if s < diff:
                    f.write(chunk0)
                    break

            # Write new header
            newOffset = header.jsonFileBegin + args.size
            newHeader = header._replace(jsonFileEnd=newOffset)
            f.seek(0)
            f.write(struct.pack(HeaderV3.fmt, *newHeader))

        elif diff < 0:
            print("Shrinking header section is not yet supported.", file=sys.stderr)
            return 1
        else:
            print("New size is identical to current size. There is nothing to do")


if __name__ == "__main__":
    sys.exit(main())
