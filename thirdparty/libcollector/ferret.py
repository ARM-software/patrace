#!/usr/bin/env python
#
# This confidential and proprietary software may be used only as
# authorised by a licensing agreement from ARM Limited
# (C) COPYRIGHT 2017 ARM Limited
# ALL RIGHTS RESERVED
# The entire notice above must be reproduced on all authorised
# copies and copies may only be made to the extent permitted
# by a licensing agreement from ARM Limited.

from __future__ import print_function
import sys
import datetime
import operator
import json
import scipy.stats as st
import argparse


# scaling of CPU frequency in kHz to MHz
#
freqScale = lambda kHz: int(kHz) / 1000.0   # scale kHz to MHz


def print_err(*args, **kwargs):
    print( *args, file=sys.stderr, **kwargs )


def process_trace( traceFile, traceProperties ):

    def process_info( properties, infoRow, state ):

        infoMap = { '_SC_CLK_TCK': lambda x: ('tick', int(x[0])),
                    'CPUList': lambda x: ('cpus', map(int, x)),
                    'WatchList': lambda x: ('watch', x),
                    'Status': lambda x: ('stat_fields', x) }

        infoType = infoRow[0]
        infoData = infoRow[1:]
        handler = infoMap[ infoType ]
        k, v = handler( infoData )

        if k == 'cpus':
            cpuNrs = v[:]

            while len( cpuNrs ) > 1:
                assert cpuNrs[0] not in cpuNrs[1:], ("Duplicate CPU number", cpuNrs[0])
                cpuNrs = cpuNrs[1:]

            v.sort()
        properties[k] = v

    def process_time( properties, timeRow, state ):

        now = float(timeRow[0]) * 1.0e-6

        ret = dict(state)

        state.clear()
        state['time'] = now

        if len(ret):
            return ret

    def process_status( properties, statusRow, state ):

        asInt = lambda x: int(x)

        convert = {'pid': asInt,
                   'comm': lambda x: x[1:-1],
                   'ppid': asInt,
                   'utime': asInt,
                   'stime': asInt,
                   'cutime': asInt,
                   'cstime': asInt,
                   'num_threads': asInt,
                   'starttime': asInt,
                   'processor': asInt}

        assert len( convert.keys() ) == len( properties['stat_fields'] )
        assert len( statusRow ) == len( properties['stat_fields'] ), statusRow

        data = [ (f, convert[f](v)) for f, v in zip( properties['stat_fields'], statusRow ) ]

        dataDict = dict( data )

        pid = dataDict['pid']

        state.setdefault( 'pid', dict() )[pid] = dataDict

    def process_cpu( properties, cpuRow, state ):

        def clamp( freq ):
            return freq if freq >= 0 else None

        cpuNrs = cpuRow[0:-1:2]
        freqs = cpuRow[1::2]
        interleave = zip( cpuNrs, freqs )

        data = [ (int(cpu), clamp( freqScale( freq ))) for cpu, freq in interleave ]

        state['cpu'] = dict( data )


    recordMap = {'I': process_info, 'T': process_time, 'S': process_status,
                 'F': process_cpu, '#': lambda x, y, z: None, 'E': lambda x, y, z: None}

    state = dict()
    for row in traceFile:
        rowSplit = row.split()
        try:
            recordType = rowSplit[0]
            rowData = rowSplit[1:]
            handler = recordMap[ recordType ]
            sample = handler( traceProperties, rowData, state )
        except:
            print( row )
            raise

        if sample:
            yield sample

    yield state


class Process(object):

    def __init__(self, schedTickHz, cpuNrs):

        self.label = None
        self.first = None
        self.last = None
        self.jiffies = None
        self.jiffyPeriod = 1.0 / schedTickHz

        self.mcyc = dict( [(k, 0) for k in cpuNrs] )
        self.totJiffies = dict( [(k, 0) for k in cpuNrs] )
        self.freqList = dict( [(k, list()) for k in cpuNrs] )

    def add(self, label, ts, jiffies, freq, cpuNr):

        assert cpuNr in self.mcyc

        self.label = label
        if not self.first:
            self.first = ts
            self.jiffies = jiffies
        self.last = ts

        diffJiffy = jiffies - self.jiffies
        self.jiffies = jiffies

        if diffJiffy and freq is None:
            assert False, "time logged to unplugged CPU at ts=%f" % ts

        self.totJiffies[cpuNr] += diffJiffy

        if freq:
            self.mcyc[cpuNr] += diffJiffy * self.jiffyPeriod * freq
            self.freqList[cpuNr].append( freq )

    def duration(self):
        return self.last - self.first

    def active_jiffies(self):
        return reduce(operator.add, self.totJiffies.values())

    def summary(self, pid):
        active = self.jiffyPeriod * self.active_jiffies()
        if not active:
            return None

        result = dict()
        result['pid'] = pid
        result['name'] = self.label
        result['duration'] = self.duration()
        result['active'] = dict( [ (k, self.jiffyPeriod * v) for k, v in self.totJiffies.items() ] )

        result['MCyc'] = self.mcyc

        result['MHz'] = dict()
        for cpuNr, freqList in self.freqList.items():
            if len( freqList ) == 0:
                result['MHz'][cpuNr] = 0
                continue
            result['MHz'][cpuNr] = st.gmean( freqList )

        return result

    def to_json(self, pid):
        ret = self.summary( pid )
        if ret:
            return json.dumps( ret )


if __name__ == "__main__":

    results = dict()

    argParser = argparse.ArgumentParser(description='Post-process burrow/ferret CPU Load data.')
    argParser.add_argument( '--json', '-j', action='store_true' )
    argParser.add_argument( '--output', '-o', type=argparse.FileType('w'), default=sys.stdout )
    argParser.add_argument( 'input', nargs='+', type=str )

    args = argParser.parse_args()
    traceNames = args.input
    cpuNrs = None

    for traceName in traceNames:

        assert traceName not in results, ("Duplicate trace file name", traceName)

        traceStart = traceEnd = None
        pidHistory = dict()
        traceProperties = dict()

        with open(traceName, 'r') as traceFile:
            for sample in process_trace( traceFile, traceProperties ):

                if not traceStart:
                    tick = traceProperties['tick']

                    assert 'cpus' in traceProperties, traceName

                    if cpuNrs:
                        # Verify cpuNrs are identical for all traces we process.
                        #
                        for a, b in zip( cpuNrs, traceProperties['cpus'] ):
                            assert a == b, "CPU sets must be identical"

                    cpuNrs = traceProperties['cpus']
                    traceStart = sample['time']

                traceEnd = sample['time']

                for pid, pidInfo in sample['pid'].items():

                    processor = pidInfo['processor']

                    assert processor in cpuNrs, "Invalid CPU"

                    freq = sample['cpu'][processor]

                    if pid not in pidHistory:
                        pidHistory[pid] = Process( tick, cpuNrs )

                    comm = pidInfo['comm']
                    ustime = pidInfo['utime'] + pidInfo['stime']

                    pidHistory[pid].add( comm, sample['time'], ustime, freq, processor)


        skipped = 0

        results[ traceName ] = list()
        for pid, h in pidHistory.items():
            if h.active_jiffies() > 0 and h.duration() > (0.01 * (traceEnd - traceStart)):
                results[ traceName ].append( h.summary( pid ) )
            else:
                skipped += 1

        print_err( "%s: %d insignificant threads skipped" % (traceName, skipped) )

    print_err()

    if args.json:
        args.output.write( json.dumps( results ) )
        args.output.write( '\n' )

    else:
        legend = ['file', 'name', 'pid', 'duration']
        keys = ['file', 'name', 'pid', 'duration', 'MCyc', 'active', 'MHz']

        # Since cpuNrs are identical for all traces we processed this produces correct
        # output.
        #
        for d in ['MCyc', 'active', 'MHz']:
            for c in cpuNrs:
                legend.append( '%s %d' % (d, c) )

        args.output.write( ", ".join( legend ) )
        args.output.write( '\n' )

        for traceName, traceResult in results.items():
            for process in traceResult:
                out = list()
                out.append( traceName )
                for l in keys[1:]:
                    if isinstance( process[l], dict ):
                        for i, j in sorted( process[l].items() ):
                            out.append( str(j) )
                    else:
                        out.append( str(process[l]) )

                args.output.write( ", ".join( out ) )
                args.output.write( '\n' )

