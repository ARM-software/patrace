#!/usr/bin/env python

###
# Basic sanity checking of analyze_trace output

import argparse
import os
import sys
import json
import csv
import re

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Validate analyze_trace against a trace to text output',
    )
    parser.add_argument('basename', help='base name of the analyze_trace input files')
    parser.add_argument('tid', help='selected thread (-1 for all)', type=int)
    args = parser.parse_args()

    basefp = open('%s.json' % args.basename, 'r')
    base = json.load(basefp)

    cmdcounts = {}
    contexts = {}

    textfp = open('%s.txt' % args.basename, 'r')
    for line in textfp:
        # [0] 0 : ret=1088 eglGetDisplay(display_id=0)
        m = re.match('\[(\w+)\] (\w+) : ret=(\w+) (\w+)\(.*', line)
        try:
            tid = int(m.group(1))
            callno = int(m.group(2))
            retval = m.group(3)
            call = m.group(4)
        except:
            continue
        if tid != args.tid and args.tid != -1:
            continue
        if not call in cmdcounts:
            cmdcounts[call] = 0
        cmdcounts[call] += 1

        if call == 'eglCreateContext':
            found = False
            for c in base['contexts']:
                if c['call_created'] == callno and c['name'] == int(retval):
                   found = True
            assert found
            contexts[retval] = {}
        elif call == 'eglDestroyContext':
            m = re.match('.*ctx=(\w+)\)', line)
            ctx = m.group(1)
            found = False
            for c in base['contexts']:
                if c['call_destroyed'] == callno and c['name'] == int(ctx):
                   found = True
            assert found
            assert ctx in contexts
            del contexts[ctx]

    assert(len(contexts) == 0)
    assert(cmdcounts['eglSwapBuffers'] == base['frames'])
    assert(base['gl_version'] == 3.2)

    sys.exit(0)
