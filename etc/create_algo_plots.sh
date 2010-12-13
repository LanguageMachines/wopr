#!/bin/sh
#
# Script to generate gnuplot files from the data file generated
# by do_algos.sh
# Needs the same parameters for TIMBL, LC and RC to work properly.
#
PLOT="DATA.ALG.plot"
#
# First, get the data from all the experiments in their
# own file.
#
for TIMBL in "-a1 +D" "-a4 +D" "-a4 +D -q1"
do
    for LC in 1 2 3
    do
	for RC in 0 1 2 3
	do
	    TSTR=l${LC}r${RC}_"${TIMBL// /}"
	    PLOTDATA=exp_${TSTR}.data
	    echo "Generating ${PLOTDATA}"
	    grep " ${TSTR}\$" ${PLOT} > ${PLOTDATA}
	done
    done
done
#
# Two ways to plot:
#   1) One algorithm, all context sizes, or
#   2) One context size, over the different algoritms.
# And then there are the different measures to plot...
#
XR="[1000:]"
YR="[]"
#${ID} ${LINES} cg cd ic pplx pplx1 mrr_cd mrr_cg mrr_gd 
#  1     2      3  4  5  6    7     8      9      10
U="using 2:3"
#
VAR="pplx"
IDX=1
for TMP in "id" "LINES" "cg" "cd" "ic" "pplx" "pplx1" "mrr_cd" "mrr_cg" "mrr_gd"
do
    if [[ $VAR == $TMP ]]
    then
	echo $IDX
	break
    else
	IDX=$(( $IDX + 1 ))
    fi
done 
U="using 2:${IDX}"
#
# First one, plot file for each algortihm, to compare
# scores for different contexts.
#
for TIMBL in "-a1 +D" "-a4 +D" "-a4 +D -q1"
do
    TSTR="${TIMBL// /}"
    GNUPLOT=gp_${TSTR}.plot
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
    for LC in 1 2 3
    do
	for RC in 0 1 2 3
	do
	    PLOTDATA=exp_l${LC}r${RC}_${TSTR}.data
	    #echo "${PLOTDATA} using 2:10 w lp"
	    # The 2:10 here is the MRR
	    PLOT="${PLOT}\"${PLOTDATA}\" ${U} w lp t \"l${LC}r${RC}\","
	done
    done
    echo ${PLOT%\,}  >>${GNUPLOT}
    #echo ${PLOT:(-1)}
done
#
# And option 2, file per context size to compare different
# algorithms.
#
for LC in 1 2 3
do
    for RC in 0 1 2 3
    do
	GNUPLOT=gp_l${LC}r${RC}.plot
	echo "Generating ${GNUPLOT}"
	echo "# autogen" >${GNUPLOT}
	
	echo "set title \"l${LC}r${RC}\"" >>${GNUPLOT}
	echo "set xlabel \"instances\""  >>${GNUPLOT}
	echo "set logscale x" >>${GNUPLOT}
	echo "set ylabel \"${VAR}\""  >>${GNUPLOT}
	echo "set grid"  >>${GNUPLOT}
	#set xtics (100,1000,10000,20000,100000) 
	
	PLOT="plot ${XR}${YR} "

	for TIMBL in "-a1 +D" "-a4 +D" "-a4 +D -q1"
	do
	    TSTR="${TIMBL// /}"
	    PLOTDATA=exp_l${LC}r${RC}_${TSTR}.data
	    # The 2:10 here is the MRR
	    PLOT="${PLOT}\"${PLOTDATA}\" ${U} w lp t \"${TSTR}\","
	done
	echo ${PLOT%\,}  >>${GNUPLOT}
    done
done
