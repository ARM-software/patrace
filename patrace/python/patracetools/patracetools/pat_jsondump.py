#!/usr/bin/env python
from __future__ import print_function
import argparse
import sys
import headerparser


def main():
    parser = argparse.ArgumentParser(
        description='Dumps the json data from a .pat trace header',
    )
    parser.add_argument('trace',
                        help='The path to the trace file')

    args = parser.parse_args()
    print(headerparser.read_json_header_as_string(args.trace))


if __name__ == "__main__":
    sys.exit(main())
