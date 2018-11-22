#!/bin/bash -e
#
# Run with full path to target patrace file and a list of frames to fastforward
#
# Before running make sure you have your public ssh key registered for on dev board.
#
# The dev board used should have paretrace installed in a virtualenv in
# /root/${patrace_version}/bin

#
# ------- SETUP -------
#

# Uncomment this for debugging
#set -x

# Change this to set patrace versions being used
patrace_stable=r2p0
patrace_latest=trunk

# Change this to the DNS name of the board used
board=root@pa-firefly-01

# Change to DDK path on dev board
ddk=ddk-nov1-dump

#
# ------- CODE -------
#

# Sanity check
if [ $# -lt 2 ]; then
	echo 'Usage: $0 <path to trace file> <space delimited list of frames>'
	exit 1
fi

STEM=`echo $1 | sed 's/$repo_path//'`
APP=`echo $STEM | awk -F/ '{print $(NF-4)}'`
VERSION=`echo $STEM | awk -F/ '{print $(NF-3)}'`
RESOLUTION=`echo $STEM | awk -F/ '{print $(NF-2)}'`
SCENE=`echo $STEM | awk -F/ '{print $(NF-1)}'`
NAME=${APP}.${SCENE}.${VERSION}

log="${NAME}.log"

rm -f *.png
ssh ${board} 'mkdir -p /root/tmp'
ssh ${board} "rm -f *.png"
scp $1 ${board}:/root/tmp/tmp.pat

# Generate all snapshots at once
highest=$2
framestring="$2/frame"
for n in "${@:3}" ; do
    ((n > highest)) && highest=$n
    framestring="$framestring,$n/frame"
done

RESULT_DIR=results
mkdir -p $RESULT_DIR

echo
echo "- Generating image from stable -"
ssh ${board} "MALI_EGL_DUMMY_DISPLAY_HEIGHT=4096 LD_LIBRARY_PATH=$ddk ${patrace_stable}/bin/paretrace -s $framestring -framerange 1 $[ $highest + 1 ] tmp/tmp.pat"
scp ${board}:/root/*.png .
for f in *.png
do
	basename=`basename $f .png`
	mv $f ${RESULT_DIR}/${NAME}.${basename}.stable.png
done

echo
echo "- Generating image from latest -"
ssh ${board} "MALI_EGL_DUMMY_DISPLAY_HEIGHT=4096 LD_LIBRARY_PATH=$ddk ${patrace_latest}/bin/paretrace -s $framestring -framerange 1 $[ $highest + 1 ] tmp/tmp.pat"
scp ${board}:/root/*.png .

echo
echo "- Comparing images -"
for f in *.png
do
	basename=`basename $f .png`
	mv $f ${RESULT_DIR}/${NAME}.${basename}.latest.png
	if ./check_compare.py ${RESULT_DIR}/${NAME}.$basename.stable.png ${RESULT_DIR}/${NAME}.$basename.latest.png ${RESULT_DIR}/${NAME}.$basename.diff.png
	then
		echo "Image verification $basename successful" | tee -a $log
	else
		echo "Image verification $basename FAILED!" | tee -a $log
	fi
done
ssh ${board} rm -f *.png

# Cleanup
mv $log $RESULT_DIR/
