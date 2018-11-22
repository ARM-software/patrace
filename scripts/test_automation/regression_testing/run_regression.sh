#!/bin/bash
#
# Master script for regression testing. This one does runs everything in the input list,
# regardless of errors encountered. This is why it is two scripts.
#
# Input: List of trace files and their fastforward frames (space delimited).
#

# Repository path
repo_path=/projects/pr368/competitive_benchmarking/trace_repository

# Sanity check
if [ $# -lt 1 ]; then
        echo "Usage: ${0##*/} <file with list of trace files and frames>"
        exit 1
fi

rm -f log.txt
rm -rf results

FILES=()
FRAMES=()
while read DIR
do
	ARG1=`echo ${DIR} | sed 's/ .*//'`
	ARGS=`echo ${DIR} | cut -d ' ' -f2-`
	FILES+=(${ARG1})
	FRAMES+=("${ARGS}")
done < $1

for DIR in "${FILES[@]}"
do
	STEM=`echo $DIR | sed 's/$repo_path//'`
	APP=`echo $STEM | awk -F/ '{print $(NF-4)}'`
	VERSION=`echo $STEM | awk -F/ '{print $(NF-3)}'`
	RESOLUTION=`echo $STEM | awk -F/ '{print $(NF-2)}'`
	SCENE=`echo $STEM | awk -F/ '{print $(NF-1)}'`
	NAME=${APP}.${SCENE}.${VERSION}
	echo | tee -a log.txt
	echo "---- $NAME : ${FRAMES[0]} ----" | tee -a log.txt
	bash ./job.sh "$DIR" ${FRAMES[0]} 2>&1 | tee -a log.txt
	FRAMES=("${FRAMES[@]:1}")
done
