#!/usr/bin/env python
import sys
import argparse
import subprocess
import os


def main():
    parser = argparse.ArgumentParser(description='Compares images from two directories by creating new images showing the differences.')
    parser.add_argument('dirin1', help='Dir to be compared')
    parser.add_argument('dirin2', help='Dir to be compared')
    parser.add_argument('dirout', help='The diffimages will end up here')
    args = parser.parse_args()

    files1 = [f for f in os.listdir(args.dirin1) if f.endswith('.png')]
    files2 = [f for f in os.listdir(args.dirin2) if f.endswith('.png')]

    files1 = sorted(files1)
    files2 = sorted(files2)

    if not os.path.exists(args.dirout):
        os.makedirs(args.dirout)

    num_tests = min(len(files1), len(files2))
    for i in range(num_tests):
        sys.stdout.write('             \r{} / {}'.format(i + 1, num_tests))
        sys.stdout.flush()

        output = subprocess.check_output(
            [
                "compare",
                "-metric", "RMSE",
                "-alpha", "Off",
                os.path.join(args.dirin1, files1[i]),
                os.path.join(args.dirin2, files2[i]),
                os.path.join(args.dirout, files1[i]),
            ],
            stderr=subprocess.STDOUT,
        )

        if output.split()[0] != '0':
            print
            print output
    print


if __name__ == "__main__":
    sys.exit(main())
