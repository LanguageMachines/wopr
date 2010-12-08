#!/bin/sh
#
TRAINBASE="reuters.martin.tok"
TESTFILE="reuters.martin.tok.1000"
WOPR="../../wopr"
SCRIPT="../../etc/do_algos.wopr_script"
LOG="README.ALG.txt"
#
CYCLE=0
#
date "+%Y-%m-%d %H:%M:%S" >> ${LOG}
#
for LINES in 9000 10000
do
    #Make the dataset here, clean up afterwards?
    TRAINFILE=${TRAINBASE}.${LINES}
    if [ -e "${TRAINFILE}" ]
    then
	echo "Data file ${TRAINFILE} exists."
    else
	echo "Creating data file ${TRAINFILE}."
	head -n ${LINES} ${TRAINBASE} > ${TRAINFILE}
    fi
    for TIMBL in "-a1 +D" "-a4 +D"
    do
	for LC in 2 3
	do
	    for RC in 0 1 2
	    do
		CYCLESTR=`printf "%03d" ${CYCLE}`
		ID=ALG${CYCLESTR}
		echo ${ID}
		echo "${ID},${TRAINFILE},timbl:'${TIMBL}',lc:${LC},rc:${RC}" >> ${LOG}
		echo ${WOPR} -s ${SCRIPT} -p trainfile:${TRAINFILE},rc:${RC},lc:${LC},id:${ID},timbl:${TIMBL},testfile:${TESTFILE}
		${WOPR} -s ${SCRIPT} -p trainset:${TRAINFILE},rc:${RC},lc:${LC},id:${ID},timbl:"${TIMBL}",testset:${TESTFILE}
		CYCLE=$(( $CYCLE + 1 ))
	    done
	done
    done
done

