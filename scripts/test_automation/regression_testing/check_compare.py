#!/usr/bin/env python
import sys
import argparse
import subprocess

def main():
    parser = argparse.ArgumentParser(description='Check output of Imagemagick compare tool')
    parser.add_argument('file1', help='Input file 1')
    parser.add_argument('file2', help='Input file 2')
    parser.add_argument('output', help='Output file')
    args = parser.parse_args()

    try:
        output = subprocess.check_output(
            [
                "compare",
                "-metric", "RMSE",
                "-alpha", "Off",
                args.file1,
                args.file2,
                args.output,
            ],
            stderr=subprocess.STDOUT,)
    except:
        sys.exit(1)
    if output.split()[0] != '0':
        sys.exit(1)
    else:
        sys.exit(0)

if __name__ == "__main__":
    main()
