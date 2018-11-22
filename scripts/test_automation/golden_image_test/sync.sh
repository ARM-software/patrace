#!/bin/bash

set -xe

: ${TRACE_REPOSITORY?"Need to set TRACE_REPOSITORY"}
: ${PATRACE_RELEASES?"Need to set PATRACE_RELEASES"}

mkdir -p ${TRACE_REPOSITORY}
rsync -arv --rsh='sshpass -f /work/jenkins/.painfra ssh -o StrictHostKeyChecking=no -l painfra' painfra@trd-login3.trondheim.arm.com:/projects/pr368/repositories/golden_image_trace_repository/* ${TRACE_REPOSITORY}/

mkdir -p ${PATRACE_RELEASES}
rsync -arv --rsh='sshpass -f /work/jenkins/.painfra ssh -o StrictHostKeyChecking=no -l painfra' painfra@trd-login3.trondheim.arm.com:/projects/pr368/patrace_releases/* ${PATRACE_RELEASES}/
