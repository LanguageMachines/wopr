#!/bin/sh
#
TRAINBASE="europarl.se.txt"
TESTFILE="europarl.se.txt.l1000"
WOPR="/exp/pberck/wopr/wopr"
SCRIPT="do_algos.wopr_script"
PXSCRIPT="/exp/pberck/wopr/etc/pplx_px.pl"
LOG="README.ALG.txt"
PLOT="DATA.ALG.plot"
#
CYCLE=1000
#
date "+#%Y-%m-%d %H:%M:%S" >> ${LOG}
#
for LINES in 1000 2000 3000 4000 5000 6000 7000 8000 9000 10000 20000 30000 40000 50000 60000 70000 80000 90000 100000
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
    for TIMBL in "-a1 +D" "-a4 +D" "-a4 +D -q1"
    do
	for LC in 1 2 3
	do
	    for RC in 0 1 2 3
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
		adc=0
		ads=0
		cg=0
		cd=0
		ic=0
		mrr_cd=0
		mrr_cg=0
		mrr_gd=0
		pplx=0
		pplx1=0
		#
		BLOB=`perl ${PXSCRIPT} -f ${PXFILE} -l ${LC} -r ${RC} | tail -n18`
		#http://www.arachnoid.com/linux/shell_programming.html

		#dist_freq sum: 25341492, ave: 1174.52
		#dist_sum sum: 104737184, ave: 4854.34
		STR=${BLOB#*dist_freq sum}
		RX=': ([0123456789]*)\, ave: \(([0123456789.]*)\)'
		if [[ "$STR" =~ $RX ]]
		then
		    adc=${BASH_REMATCH[2]}
		fi
		#
		STR=${BLOB#*dist_sum sum}
		RX=': (.*)\, ave: \(([0123456789.]*)\)'
		if [[ "$STR" =~ $RX ]]
		then
		    ads=${BASH_REMATCH[2]}
		fi
		#
		STR=${BLOB#*Column:  2}
		RX='.* \((.*)%\).*3.* \((.*)%\).*4.* \((.*)%\).*'
		if [[ "$STR" =~ $RX ]]
		then
		    cg=${BASH_REMATCH[1]}
		    cd=${BASH_REMATCH[2]}
		    ic=${BASH_REMATCH[3]}
		fi
		#echo ${BLOB}
		STR=${BLOB#*Wopr ppl}
		RX=': (.*) Wopr ppl1: (.*) \(.*'
		#
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
		TSTR=l${LC}r${RC}_"${TIMBL// /}"
		printf -v S "%s %s %s %s %s %s %s %s %s %s %s %s %s" ${ID} ${LINES} ${cg} ${cd} ${ic} ${pplx} ${pplx1} ${mrr_cd} ${mrr_cg} ${mrr_gd} ${adc} ${ads} ${TSTR}
		echo ${S} >> ${PLOT}
	    done
	done
    done
done

