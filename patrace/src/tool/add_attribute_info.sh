#!/usr/bin/env bash

## Script that patches trace files, by add information about their attributes
## into the header.

set -eu

## This should point to a result file (a result file from a sweep or retrace)
## A directory called patches will be generated based on this content.
RESULT_FILE=$1

## Let PREPEND_PATH be an empty string, "", to operate DIRECTLY ON THE TRACE REOPOSITORY
##
## !!!PLEASE USE WITH EXTREME CAUTION!!!
##
PREPEND_PATH="."

## This should point the patrace install directory
PATRACE_DIR="/scratch/joasim01/cluster/patrace/10.04_x11_x64/"

PYTHON_TOOLS="python ${PATRACE_DIR}/python_tools"

echoerr() { echo "$@" 1>&2; }

## Create Patches
$PYTHON_TOOLS/attribute_result_to_header.py $RESULT_FILE

## Generate file list
find -name "*.patch.json" > files.txt

cat ./files.txt | while read i; do
    ## Generate .pat file name
    PAT=${i#"./patches"}
    PAT=${PAT%".patch.json"}
    PAT="${PREPEND_PATH}${PAT}"

    if [ ! -e $PAT ]; then
        echo "WARNING: $PAT does not exist"
        continue
    fi

    #if [[ $PAT == /projects/pr368/competitive_benchmarking/trace_repository/internal/android/TempleRun2/1.6/1920x1080/normal_gameplay/* ]]; then
    #    echo "IGNORING: $PAT"
    #    continue
    #fi

    #if [[ $PAT == /projects/pr368/competitive_benchmarking/trace_repository/internal/android/TempleRun/1.0.8/1920x1080/normal_gameplay/* ]]; then
    #    echo "IGNORING: $PAT"
    #    continue
    #fi

    ## Make backups
    if [ ! -e $PAT.bak_joakimpatch ]; then
        cp -v $PAT $PAT.bak_joakimpatch
        cp -v "${PAT%.*}.meta" "${PAT%.*}.meta.bak_joakimpatch"
    fi

    find `dirname $PAT` -name "*.pat" > all_files_for_test.txt

    cat ./all_files_for_test.txt | while read PAT; do
        if [ -e $PAT.patching_ok ]; then
            echo "ALREADY PATCHED: $PAT"
            continue
        fi


        if [ -e $PAT ]; then
            VERSION=`$PYTHON_TOOLS/pat_version.py $PAT`

            if [ "$VERSION" -lt 3 ]; then
                echo "$PAT has too low version: $VERSION"
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

            echo "Patching $PAT... with $i"

            ## Increase header sizes
            mv "$PAT" "$PAT.inprocess"
            $PYTHON_TOOLS/set_header_size.py "$PAT.inprocess" 524256
            mv "$PAT.inprocess" "$PAT"

            ## Merge patches
            $PYTHON_TOOLS/edit_header.py $PAT --mergeattrpatch $i

            ## Update MD5 sums
            $PYTHON_TOOLS/update_md5.py $PAT

            touch $PAT.patching_ok
        else
            echo "File not found: $PAT"
        fi
    done
done
