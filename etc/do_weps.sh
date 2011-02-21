#!/bin/sh
#
TRAINFILE="rmt.5e3"
DEVFILE="rmt.t1e2"
TESTFILE="rmt.t1e2b"
FOCUS="rmt.focus"
#
WOPR="wopr"
CYCLE=000
SCRIPT="./wep.wopr_script"
#
for FFC in 100 1000 10000
do
    CYCLESTR=`printf "%03d" ${CYCLE}`
    ID=RUN${CYCLESTR}.ffc${FFC}
    FOCUSNEW=${FOCUS}_${CYCLESTR}
    cp ${FOCUS} ${FOCUSNEW}
    echo "${WOPR} -l -s ${SCRIPT} -p filename:${TRAINFILE},testfile:${TESTFILE},devfile:${DEVFILE},focus:${FOCUSNEW},ffc:${FFC},id:${ID}"
    ${WOPR} -l -s ${SCRIPT} -p filename:${TRAINFILE},testfile:${TESTFILE},devfile:${DEVFILE},focus:${FOCUSNEW},ffc:${FFC},id:${ID} # > output.${ID}`
    CYCLE=$(( $CYCLE + 1 ))
done
