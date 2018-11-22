#!/usr/bin/env python
import argparse
import os
import json
import hashlib
import patracetools.utils

def main():
    parser = argparse.ArgumentParser(description="Update the md5 sum for the meta file of a .pat trace.")
    parser.add_argument('file', help='Path to the .pat trace file')
    args = parser.parse_args()

    metaname = os.path.splitext(args.file)[0] + ".meta"

    jsondata = json.load(open(metaname)) if os.path.exists(metaname) else {}

    print("Calculating md5 sum for {f}".format(f=args.file))
    jsondata['md5'] = patracetools.utils.md5sum(args.file)

    json.dump(jsondata, open(metaname, 'w'), indent=2, sort_keys=True)

if __name__ == '__main__':
    main()
