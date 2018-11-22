#!/usr/bin/env bash

## Script that executes a command recursively on valid trace files.
## Usage 
set -eu

## Command that will be executed
COMMAND="$1"

## This should point to the trace repository 
TRACE_REPO="$2"

## This should point the patrace install directory
PATRACE_DIR="/scratch/joasim01/cluster/patrace/10.04_x11_x64/"

PYTHON_TOOLS="python ${PATRACE_DIR}/python_tools"

echoerr() { echo "$@" 1>&2; }

## Generate file list
find ${TRACE_REPO} -name "*.pat" > /tmp/files.txt

cat /tmp/files.txt | while read i; do
    ## Generate .pat file name
    PAT="${i}"

    #echo "Processing trace: ${PAT}"

    if [ -e $PAT ]; then
        VERSION=`$PYTHON_TOOLS/pat_version.py $PAT`

        if [ "$VERSION" -lt 3 ]; then
            echoerr "$PAT has too low version: $VERSION"
            continue
        fi

        set +e
        ${PATRACE_DIR}/bin/eglretrace -infojson $PAT > /dev/null 2>&1
        rc=$?
        set -e

        if [[ $rc != 0 ]]; then
            echoerr "Problem opening ${PAT}"
            continue
        fi
        

        ## Count calls
        set +e
        eval $COMMAND $PAT
        set -e

    else
        echoerr "File not found: $PAT"
    fi
done
