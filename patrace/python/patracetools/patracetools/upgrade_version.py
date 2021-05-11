#!/usr/bin/env python3


import argparse
import sys
import os
import json
import datetime
from patracetools import pat_version

try:
    from patrace import InputFile, OutputFile
except ImportError:
    print('patrace (Python interface of PATrace SDK) is required.')

import patracetools.version
import patracetools.utils

class Upgrader:
    def __init__(self, args):
        self.num_calls = 0
        self.args = args

    def run(self, input, output):
        header = json.loads(input.jsonHeader)

        # Parse out version
        with open(self.args.file, "rb") as f:
            hh = pat_version.HeaderHead.make(f)

        if hh.magicNo != 538058770:
            raise ValueError("{f} is not a .pat file".format(f=self.args.file))

        inputVersion = hh.version - 2
        inputMD5 = patracetools.utils.md5sum(self.args.file)

        if not "conversions" in header:
             header["conversions"] = []

        path = os.path.abspath(self.args.file)

        header["conversions"].append({
            "type": "upgradeVersion",
            "timestamp": datetime.datetime.utcnow().isoformat(),
            "patraceVersion": patracetools.version.version_full,
            "input": {
                "md5" : inputMD5,
                "file": path,
            },
            "versionInfo": {
                "from" : inputVersion,
                "to"   : output.version,
            },
        })

        output.jsonHeader = json.dumps(header)

        for call in input.Calls():
            output.WriteCall(call)
            self.num_calls = self.num_calls + 1

def main():
    parser = argparse.ArgumentParser(description='Upgrade .pat trace file to use the newest binary format')
    parser.add_argument('file', help='Path to the .pat trace file')
    parser.add_argument('--newfile', default='trace_out.pat', help="Specifies the path to the output trace file")

    args = parser.parse_args()
    upgrader = Upgrader(args)

    with InputFile(args.file) as input:
        with OutputFile(args.newfile) as output:
            upgrader.run(input, output)

    print("Number of calls in trace {num}".format(num=upgrader.num_calls))

if __name__ == '__main__':
    main()
