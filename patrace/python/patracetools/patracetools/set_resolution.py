#!/usr/bin/env python2
import argparse
import json

try:
    from patrace import InputFile, OutputFile
except ImportError:
    print 'patrace (Python interface of PATrace SDK) is required.'


def main():
    parser = argparse.ArgumentParser(description='Change the resolution for all threads in trace')
    parser.add_argument('file', help='Path to the .pat trace file')
    parser.add_argument('width', type=int, help='Resolution width in pixels')
    parser.add_argument('height', type=int, help='Resolution height in pixels')
    parser.add_argument('--newfile', default='trace_out.pat', help="Specifies the path to the output trace file")
    args = parser.parse_args()

    with InputFile(args.file) as input:
        with OutputFile(args.newfile) as output:
            # Modify header, if we are remaping the default tid
            header = json.loads(input.jsonHeader)
            for thread in header['threads']:
                thread['winH'] = args.height
                thread['winW'] = args.width
            output.jsonHeader = json.dumps(header)

            for call in input.Calls():
                output.WriteCall(call)

if __name__ == '__main__':
    main()
