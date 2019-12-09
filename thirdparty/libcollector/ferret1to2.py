#!/usr/bin/env python
#
# This confidential and proprietary software may be used only as
# authorised by a licensing agreement from ARM Limited
# (C) COPYRIGHT 2017 ARM Limited
# ALL RIGHTS RESERVED
# The entire notice above must be reproduced on all authorised
# copies and copies may only be made to the extent permitted
# by a licensing agreement from ARM Limited.

# Script to convert procwatch/ferret v1 to libcollector-based v2

import sys
import os

# rewritten files to a subdirectory to avoid name conflicts
#
outDir = './ferret2'

# v2 requires these fields
#
statFields = "pid comm ppid utime stime cutime cstime num_threads starttime processor"

if __name__ == "__main__":

	traceNames = sys.argv[1:]

	if not os.path.exists( outDir ):
		os.mkdir( outDir )

	for traceName in traceNames:

		outFile = os.path.join( outDir, traceName )

		with open(traceName, 'r') as traceFile:

			with open(outFile, 'w') as newTraceFile:

				header = list()
				header.append( "# Converted by ferret1to2.py" )
				header.append( "I _SC_CLK_TCK 100" )
				header.append( "I Status %s" % statFields )
				header.append( "" )

				header = "\n".join( header )
				newTraceFile.write( header )

				cpuList = None

				for row in traceFile:
					records = row.split(',')

					if len(records) < 3:
						# v1 had a habit of writing partial sample rows
						# when interrupted.
						#
						continue

					assert len(records) > 1, row

					# v2 timestamps are integer microseconds, not seconds with microsecond
					# resolution
					#
					now = int( float(records[0]) * 1.0e6 )
					status = list()
					cpuFreq = list()

					for r in records[1:]:

						if r[0] == 's':

							state = r.split()[1:]
							del state[2]	# delete "state" field from stat
							status.append( " ".join( state ) )

						elif r[0] == 'f':

							raw = r.split()[1:]
							cpus = raw[0:-1:2]
							freqs = raw[1::2]

							# cpus are labelled cpu[0-9]+ so just extract the digit(s)
							#
							cpus = [ c[3:] for c in cpus]

							if not cpuList:
								# use the first CPU DVFS record to declare the CPU set being
								# sampled
								#
								cpuList = cpus[:]
								newTraceFile.write( "I CPUList %s\n" % " ".join( cpus ) )

							# and put it all back together again
							#
							cpuFreq.append( " ".join( sum( zip( cpus, freqs ), () ) ) )

						else:
							assert False, ("Unrecognised record", r[0], row)

					assert len(cpuFreq) == 1, (row, cpuFreq)

					newTraceFile.write( "T %s\n" % now )
					newTraceFile.write( "F %s\n" % cpuFreq[0] )
					for s in status:
						newTraceFile.write( "S %s\n" % s )
