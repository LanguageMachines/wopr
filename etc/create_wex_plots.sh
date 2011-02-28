#!/bin/sh
#
# Script to generate gnuplot files from the data file generated
# by do_meta_nyt.sh.
#
#WEX10112 100000 l2r0 1 1 cg 35315 94039 103473 0.41 35392 89803 107632 0.46
#WEX10113 100000 l2r1 2 1 cg 53663 69138 110026 0.63 55029 68436 109362 0.68
#WEX10114 100000 l3r0 1 1 cg 34866 75394 122567 0.47 35096 73296 124435 0.54
#
PREFIX="pre"
VAR="cg"
if test $# -lt 1
then
  echo "Supply DATAFILE, (and PREFIX)"
  exit
fi
#
DATA=$1
if test $# -eq 2
then
    PREFIX=$2
fi
#
# Determine algos used from file.
# Determine the LC and RC used.
# Format:
# WEX10114 100000 l3r0 1 1 cg 34866 75394 122567 0.47 35096 73296 124435 0.54
#
LCS=`cut -d' ' -f 3 ${DATA}  | cut -c2 | sort -u`
RCS=`cut -d' ' -f 3 ${DATA}  | cut -c4 | sort -u`
#
# First, get the data from all the experiments in their
# own file.
#
for LC in ${LCS}
do
    for RC in ${RCS}
    do
	CTXSTR=l${LC}r${RC}
	PLOTDATA=${PREFIX}_${CTXSTR}_${VAR}.data
	echo "Generating ${PLOTDATA}"
	egrep " ${CTXSTR} (.*?) (.?*) ${VAR}" ${DATA} > ${PLOTDATA}
    done
done
#
#
#
XR="[1000:]"
YR="[0:500]"
#
# File per context size
#
for LC in ${LCS}
do
    for RC in ${RCS}
    do
	CTXSTR=l${LC}r${RC}_${VAR}
	GNUPLOT=${PREFIX}_${CTXSTR}.plot
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

	#loop over cg cd ic, ..._${VAR}.data ?
	PLOTDATA=${PREFIX}_${CTXSTR}.data
	# Add if it exists
	if [[ -s ${PLOTDATA} ]]
	then
	    PLOT="${PLOT}\"${PLOTDATA}\" ${U} w lp t \"${TSTR}\","
	fi
	echo ${PLOT%\,} >>${GNUPLOT}
    done
done
#
#set terminal push
#set terminal postscript eps color lw 2 "Helvetica" 10
#set out 'l2r0_-a1.ps'
#replot
#set term pop