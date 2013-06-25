#!/bin/sh
#
# Script to generate gnuplot files from the data file generated
# by do_algos.sh
# Needs the same parameters for TIMBL, LC and RC to work properly.
#
#LSR="noenhanced" #"color"
LSR="enhanced color solid rounded"
#
PREFIX="exp"
if test $# -lt 2
then
  echo "Supply FILE, VAR (and PREFIX)"
  exit
fi
#
PLOT=$1
VAR=$2
ALLDATA=$1
if test $# -eq 3
then
    PREFIX=$3
fi
#
# AL or SC
#TYP=`cut -c1-2 ${PLOT}`
TYP=${PLOT:5:2}
#
if [ "$TYP" == "AL" -o "$TYP" == "" -o "$TYP" == "MR" ]
then
    ALGOS=`cut -d' ' -f 13 ${PLOT}  | cut -c6- | sort -u`
    LCS=`cut -d' ' -f 13 ${PLOT}  | cut -c2 | sort -u`
    RCS=`cut -d' ' -f 13 ${PLOT}  | cut -c4 | sort -u`
    VARS='id LINES cg cd ic pplx pplx1 mrr_cd mrr_cg mrr_gd adc ads'
    VAREND=13
else #SC or DT
    ALGOS=`cut -d' ' -f 13 ${PLOT}  | cut -c6- | sort -u`
    LCS=`cut -d' ' -f 13 ${PLOT}  | cut -c2 | sort -u`
    RCS=`cut -d' ' -f 13 ${PLOT}  | cut -c4 | sort -u`
    VARS='id LINES lines errors gsa bsa wsa nsa gs bs ws ns'
    #            good sugg, bad sugg, wrong sugg, no sugg, (absolute)
    VAREND=4
fi
echo $VARS
#
IDX=1
#for TMP in "id" "LINES" "cg" "cd" "ic" "pplx" "pplx1" "mrr_cd" "mrr_cg" "mrr_gd" "adc" "ads"
for TMP in $VARS
do
    if [[ $VAR == $TMP ]]
    then
	break
    fi
    IDX=$(( $IDX + 1 ))
done 
if test $IDX -eq $VAREND
then
    echo "ERROR, unknown variable ${VAR}"
    exit 1
fi
PREFIX=${PREFIX}_${VAR}
#
# Determine algos used from file.
# Determine the LC and RC used.
# Format:
# ALG1000 1000 9.67 42.99 47.34 188.67 188.67 0.188 1.000 0.337 12 22 l1r0_-a1+D
# ALG50004 1000 10.93 29.60 59.47 214.49 214.49 0.203 1.000 0.418 1269.02 6118.77 l3r0_-a1+D_hpx0
# SC10024 5000 22829 1785 21 12 1 1763 1.18 0.67 0.06 98.77 l2r0_-a1+D
#
#
echo ${ALGOS}
#
# First, get the data from all the experiments in their
# own file.
#
for TIMBL in ${ALGOS}
do
    for LC in ${LCS}
    do
	for RC in ${RCS}
	do
	    #if [[ ${LC} -ne "0" -o ${RC} -ne "0" ]]
	    if test ${LC} -eq "0" -a ${RC} -eq "0"
	    then
		echo "Skipping l${LC}r${RC}"
		continue
	    fi
	    TSTR=l${LC}r${RC}_"${TIMBL// /}"
	    PLOTDATA=${PREFIX}_${TSTR}.data
	    echo "Generating ${PLOTDATA}"
	    sort -u ${PLOT} | grep " ${TSTR}\$"  | egrep -v " 0 0 0 " > ${PLOTDATA}
	    # should check if empty here? And delete straight away?
	done
    done
done
#
# Two ways to plot:
#   1) One algorithm, all context sizes, or
#   2) One context size, over the different algorithms.
# And then there are the different measures to plot...
#
XR="[1000:]"
#${ID} ${LINES} cg cd ic pplx pplx1 mrr_cd mrr_cg mrr_gd adc ads ALG
#  1     2      3  4  5  6    7     8      9      10     11  12
#U="using 2:3"
#
U="using 2:${IDX}"
YR="[0:500]"
if [[ ${VAR:0:3} == "mrr" ]]
then
    YR="[0:1]"
fi
if [[ ${VAR:0:6} == "mrr_cd" ]]
then
    YR="[0:0.5]"
fi
if [[ ${VAR:0:4} == "pplx" ]]
then
    YR="[0:700]"
fi
if [[ ${IDX} -lt 5 ]]
then
    YR="[0:50]"
fi
if [[ ${VAR:0:2} == "ic" ]]
then
    YR="[20:100]"
fi
if [[ ${VAR:0:2} == "cg" ]]
then
    YR="[0:40]"
fi
if [[ ${VAR:0:3} == "adc" ]]
then
    YR="[0:8000]"
fi
if [[ ${VAR:0:3} == "ads" ]]
then
    YR="[]"
fi
#
if [ "$TYP" == "SC" -o "$TYP" == "DT" ]
then
    YR="[]"
    if [[ ${VAR:0:2} == "gs" ]]
    then
	YR="[]"
    fi
fi
#
# First one, plot file for each algorithm, to compare
# scores for different contexts.
#
#for TIMBL in "-a1 +D" "-a4 +D" "-a4 +D -q1"
for TIMBL in ${ALGOS}
do
    TSTR="${TIMBL// /}"
    GNUPLOT=${PREFIX}_${TSTR}.plot
    echo "Generating ${GNUPLOT}"
    echo "# autogen" >${GNUPLOT}

	echo "set style line 1 lt -1 lw 0.5 pi -1 pt 7 ps 0.4" >>${GNUPLOT}
	echo "set style line 2 lt -1 lw 0.5 pi -1 pt 6" >>${GNUPLOT}
	echo "set style line 3 lt -1 lw 0.5 pi -1 pt 7" >>${GNUPLOT}
	echo "set style line 4 lt 2 lw 0.5 pi -1 pt 7 ps 0.4" >>${GNUPLOT}

    echo "set title \"${TSTR}\"" >>${GNUPLOT}
    echo "set xlabel \"lines of data\""  >>${GNUPLOT}
    echo "set key bottom"  >>${GNUPLOT}
    echo "set logscale x" >>${GNUPLOT}
    echo "set ylabel \"${VAR}\""  >>${GNUPLOT}
    echo "#set grid" >>${GNUPLOT}
	echo "set border 3" >>${GNUPLOT}
	echo "set xtics out nomirror" >>${GNUPLOT}
	echo "set ytics out nomirror" >>${GNUPLOT}
	echo 'set xtics ("1,000" 1000,"10,000" 10000,"100,000" 100000,"1,000,000" 1000000,"10,000,000" 10000000)' >>${GNUPLOT}
    #set xtics (100,1000,10000,20000,100000) 

	LS=1 #linestyle
    PLOT="plot ${XR}${YR} "
    for LC in ${LCS}
    do
	for RC in ${RCS}
	do
	    if test ${LC} -eq "0" -a ${RC} -eq "0"
	    then
		echo "Skipping l${LC}r${RC}"
		continue
	    fi
	    PLOTDATA=${PREFIX}_l${LC}r${RC}_${TSTR}.data
	    #echo "${PLOTDATA} using 2:10 w lp"
	    # Add if it exists.
	    if [[ -s ${PLOTDATA} ]]
	    then
			PLOT="${PLOT}\"${PLOTDATA}\" ${U} w lp ls ${LS} t \"l${LC}r${RC}\","
			LS=$(( $LS + 1 ))
	    fi
	done
    done
    echo ${PLOT%\,}  >>${GNUPLOT}
    echo ${PLOT%\,}  >>${GNUPLOT}
    echo "set terminal push" >>${GNUPLOT}
    echo "#set terminal postscript eps ${LSR} lw 2 'Helvetica' 10" >>${GNUPLOT}
	echo "set terminal postscript eps enhanced dashed rounded lw 2 'Helvetica' 10" >>${GNUPLOT}
    PSPLOT=${PREFIX}_${TSTR}.ps
    echo "set out '${PSPLOT}'" >>${GNUPLOT}
    echo "replot" >>${GNUPLOT}
    echo "!epstopdf '${PSPLOT}'" >> ${GNUPLOT}
    echo "set term pop" >>${GNUPLOT}
    #echo ${PLOT:(-1)}
done
#
# And option 2, file per context size to compare different
# algorithms.
#
for LC in ${LCS}
do
    for RC in ${RCS}
    do
	if test ${LC} -eq "0" -a ${RC} -eq "0"
	then
	    echo "Skipping l${LC}r${RC}"
	    continue
	fi
	GNUPLOT=${PREFIX}_l${LC}r${RC}.plot
	echo "Generating ${GNUPLOT}"
	echo "# autogen" >${GNUPLOT}
	
	echo "set style line 1 lt -1 lw 0.5 pi -1 pt 7 ps 0.4" >>${GNUPLOT}
	echo "set style line 2 lt -1 lw 0.5 pi -1 pt 6" >>${GNUPLOT}
	echo "set style line 3 lt -1 lw 0.5 pi -1 pt 7" >>${GNUPLOT}
	echo "set style line 4 lt 2 lw 0.5 pi -1 pt 7 ps 0.4" >>${GNUPLOT}

	echo "set title \"l${LC}r${RC}\"" >>${GNUPLOT}
	echo "set xlabel \"lines of data\""  >>${GNUPLOT}
	echo "set key bottom"  >>${GNUPLOT}
	echo "set logscale x" >>${GNUPLOT}
	echo "set ylabel \"${VAR}\""  >>${GNUPLOT}
	echo "#set grid"  >>${GNUPLOT}
	echo "set border 3" >>${GNUPLOT}
	echo "set xtics out nomirror" >>${GNUPLOT}
	echo "set ytics out nomirror" >>${GNUPLOT}
	echo 'set xtics ("1,000" 1000,"10,000" 10000,"100,000" 100000,"1,000,000" 1000000,"10,000,000" 10000000)' >>${GNUPLOT}

	#set xtics (100,1000,10000,20000,100000) 

	LS=1 #linestyle
	PLOT="plot ${XR}${YR} "

	#for TIMBL in "-a1 +D" "-a4 +D" "-a4 +D -q1"
	for TIMBL in ${ALGOS}
	do
	    TSTR="${TIMBL// /}"
	    PLOTDATA=${PREFIX}_l${LC}r${RC}_${TSTR}.data
	    # Add if it exists
	    if [[ -s ${PLOTDATA} ]]
	    then
			PLOT="${PLOT}\"${PLOTDATA}\" ${U} w lp ls ${LS} t \"${TSTR}\","
			LS=$(( $LS + 1 ))
	    fi
	done
	echo ${PLOT%\,}  >>${GNUPLOT}
	echo "set terminal push" >>${GNUPLOT}
	echo "#set terminal postscript eps ${LSR} lw 2 'Helvetica' 10" >>${GNUPLOT}
	echo "set terminal postscript eps enhanced dashed rounded lw 2 'Helvetica' 10" >>${GNUPLOT}
	PSPLOT=${PREFIX}_l${LC}r${RC}.ps
	echo "set out '${PSPLOT}'" >>${GNUPLOT}
	echo "replot" >>${GNUPLOT}
	echo "!epstopdf '${PSPLOT}'" >> ${GNUPLOT}
	echo "set term pop" >>${GNUPLOT}
    done
done
#
#set terminal push
#set terminal postscript eps color lw 2 "Helvetica" 10
#set out 'l2r0_-a1.ps'
#replot
#set term pop
