#!/bin/sh
#
# Script to generate gnuplot files from the data file generated
# by do_algos.sh
# Needs the same parameters for TIMBL, LC and RC to work properly.
#
LSR="noenhanced" #"color"
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
if test $# -eq 3
then
    PREFIX=$3
fi
#
IDX=1
for TMP in "id" "LINES" "cg" "cd" "ic" "pplx" "pplx1" "mrr_cd" "mrr_cg" "mrr_gd" "adc" "ads"
do
    if [[ $VAR == $TMP ]]
    then
	break
    fi
    IDX=$(( $IDX + 1 ))
done 
if test $IDX -eq 13
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
#
ALGOS=`cut -d' ' -f 13 ${PLOT}  | cut -c6- | sort -u`
LCS=`cut -d' ' -f 13 ${PLOT}  | cut -c2 | sort -u`
RCS=`cut -d' ' -f 13 ${PLOT}  | cut -c4 | sort -u`
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
	    TSTR=l${LC}r${RC}_"${TIMBL// /}"
	    PLOTDATA=${PREFIX}_${TSTR}.data
	    echo "Generating ${PLOTDATA}"
	    grep " ${TSTR}\$" ${PLOT} > ${PLOTDATA}
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
if [[ ${VAR:0:4} == "pplx" ]]
then
    YR="[0:800]"
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
    YR="[0:4000]"
fi
if [[ ${VAR:0:3} == "ads" ]]
then
    YR="[]"
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

    echo "set title \"${TSTR}\"" >>${GNUPLOT}
    echo "set xlabel \"instances\""  >>${GNUPLOT}
    echo "set key bottom"  >>${GNUPLOT}
    echo "set logscale x" >>${GNUPLOT}
    echo "set ylabel \"${VAR}\""  >>${GNUPLOT}
    echo "set grid"  >>${GNUPLOT}
    #set xtics (100,1000,10000,20000,100000) 

    PLOT="plot ${XR}${YR} "
    for LC in ${LCS}
    do
	for RC in ${RCS}
	do
	    PLOTDATA=${PREFIX}_l${LC}r${RC}_${TSTR}.data
	    #echo "${PLOTDATA} using 2:10 w lp"
	    # Add if it exists.
	    if [[ -s ${PLOTDATA} ]]
	    then
		PLOT="${PLOT}\"${PLOTDATA}\" ${U} w lp t \"l${LC}r${RC}\","
	    fi
	done
    done
    echo ${PLOT%\,}  >>${GNUPLOT}
    echo ${PLOT%\,}  >>${GNUPLOT}
    echo "set terminal push" >>${GNUPLOT}
    echo "set terminal postscript eps ${LSR} lw 2 'Helvetica' 10" >>${GNUPLOT}
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
	GNUPLOT=${PREFIX}_l${LC}r${RC}.plot
	echo "Generating ${GNUPLOT}"
	echo "# autogen" >${GNUPLOT}
	
	echo "set title \"l${LC}r${RC}\"" >>${GNUPLOT}
	echo "set xlabel \"instances\""  >>${GNUPLOT}
	echo "set key bottom"  >>${GNUPLOT}
	echo "set logscale x" >>${GNUPLOT}
	echo "set ylabel \"${VAR}\""  >>${GNUPLOT}
	echo "set grid"  >>${GNUPLOT}
	#set xtics (100,1000,10000,20000,100000) 
	
	PLOT="plot ${XR}${YR} "

	#for TIMBL in "-a1 +D" "-a4 +D" "-a4 +D -q1"
	for TIMBL in ${ALGOS}
	do
	    TSTR="${TIMBL// /}"
	    PLOTDATA=${PREFIX}_l${LC}r${RC}_${TSTR}.data
	    # Add if it exists
	    if [[ -s ${PLOTDATA} ]]
	    then
		PLOT="${PLOT}\"${PLOTDATA}\" ${U} w lp t \"${TSTR}\","
	    fi
	done
	echo ${PLOT%\,}  >>${GNUPLOT}
	echo "set terminal push" >>${GNUPLOT}
	echo "set terminal postscript eps ${LSR} lw 2 'Helvetica' 10" >>${GNUPLOT}
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