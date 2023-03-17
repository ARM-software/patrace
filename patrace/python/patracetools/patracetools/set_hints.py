#!/usr/bin/env python3
import argparse
import os
import sys
import datetime
import shutil
from patracetools import headerparser
import patracetools.version


def main():
    parser = argparse.ArgumentParser(description='Add hints as new entry in the conversions list in the trace header.')
    parser.add_argument('src', help='Path to the original .pat trace file')
    parser.add_argument('dst', help='Path to the new .pat trace file')
    hint_args = parser.add_argument_group('hints')
    hint_args.add_argument(
        '--firstFrame',
        help='Indicates which frame number the first frame in the trace represents. Can by any integer.',
        type=int,
    )
    args = parser.parse_args()

    header = headerparser.read_json_header(args.src)

    hints = {}

    if args.firstFrame is not None:
        hints.update({'firstFrame': args.firstFrame})

    if not hints:
        print("Please specify at least one argument hint argument.")
        sys.exit(1)

    header['conversions'] = (
        header.get('conversions', []) + [
            {
                'author': os.getlogin(),
                'hints': hints,
                'timestamp': datetime.datetime.utcnow().isoformat(),
                'tool': 'pat-set-hints',
                'version': patracetools.version.version_full,
            }
        ]
    )

    print(f'Writing {args.dst}...')
    shutil.copyfile(args.src, args.dst)
    headerparser.write_json_header(args.dst, header)


if __name__ == '__main__':
    main()
