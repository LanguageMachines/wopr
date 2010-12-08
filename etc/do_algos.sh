#!/bin/sh
#
TRAINBASE="reuters.martin.tok"
TESTFILE="austen.l100"
WOPR="../../wopr"
SCRIPT="../../etc/do_algos.wopr_script"
PXSCRIPT="../../etc/pplx_px.pl"
LOG="README.ALG.txt"
#
CYCLE=1000
#
date "+#%Y-%m-%d %H:%M:%S" >> ${LOG}
#
for LINES in 5000
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
    for TIMBL in "-a1 +D" "-a1 +D -w0" "-a4 +D" "-a4 +D -w0" "-a4 +D -q1"
    do
	for LC in 2 3
	do
	    for RC in 0 
	    do
		CYCLESTR=`printf "%04d" ${CYCLE}`
		ID=ALG${CYCLESTR}
		echo ${ID}
		echo "#${ID},${TRAINFILE},timbl:'${TIMBL}',lc:${LC},rc:${RC}" >> ${LOG}
		echo ${WOPR} -s ${SCRIPT} -p trainfile:${TRAINFILE},rc:${RC},lc:${LC},id:${ID},timbl:${TIMBL},testfile:${TESTFILE}
		${WOPR} -s ${SCRIPT} -p trainset:${TRAINFILE},rc:${RC},lc:${LC},id:${ID},timbl:"${TIMBL}",testset:${TESTFILE}
		CYCLE=$(( $CYCLE + 1 ))
		#
		#reuters.martin.tok.1000.l3r2_ALG023.px
		PXFILE=${TESTFILE}.l${LC}r${RC}_${ID}.px
		#http://wiki.bash-hackers.org/commands/builtin/printf
		#printf -v S "%s %s" ${ID} ${PXFILE}
		#echo $S
		mrr_cd=0
		mrr_cg=0
		mrr_gd=0
		pplx=0
		pplx1=0
		BLOB=`perl ${PXSCRIPT} -f ${PXFILE} -l ${LC} -r ${RC} | tail -n10`
		#echo ${BLOB}
		STR=${BLOB#*Wopr ppl}
		RX=': (.*) Wopr ppl1: (.*) \(.*'
		if [[ "${STR}" =~ $RX ]]
		then
		    pplx=${BASH_REMATCH[1]}
		    pplx1=${BASH_REMATCH[2]}
		fi
		#
		STR=${BLOB#*RR(cd)}
		#echo $STR
		RX='.* MRR: (.*).*RR\(cg\).* MRR: (.*).*RR\(gd\).* MRR: (.*)'
		if [[ "${STR}" =~ $RX ]]
		then
		    mrr_cd=${BASH_REMATCH[1]}
		    mrr_cg=${BASH_REMATCH[2]}
		    mrr_gd=${BASH_REMATCH[3]}
		fi
		printf -v S "%s %s %s %s %s %s %s" ${ID} ${LINES} ${pplx} ${pplx1} ${mrr_cd} ${mrr_cg} ${mrr_gd}
		echo ${S} >> ${LOG}
	    done
	done
    done
done

