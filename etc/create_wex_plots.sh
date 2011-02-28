#!/bin/sh
#
# Script to generate gnuplot files from the data file generated
# by do_meta_nyt.sh.
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
# WEX10321 500000 l3r1 2 1 cg 65433 68482 98912 0.69 66377 64408 102042 0.78 -a1+D -a4+D 1-500
LCS=`cut -d' ' -f 3 ${DATA}  | cut -c2 | sort -u`
RCS=`cut -d' ' -f 3 ${DATA}  | cut -c4 | sort -u`
T0S=`cut -d' ' -f 15 ${DATA}  | sort -u`
T1S=`cut -d' ' -f 16 ${DATA}  | sort -u`
RS=`cut -d' ' -f 17 ${DATA}  | sort -u`
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
U0="using 2:7"
U1="using 2:11"
XR="[9000:]"
YR="[]"
#
# File per context size
#
for LC in ${LCS}
do
    for RC in ${RCS}
    do
	CTXSTR=l${LC}r${RC}_${VAR}
	GNUPLOT=${PREFIX}_${CTXSTR}.plot
	PSPLOT=${PREFIX}_${CTXSTR}.ps
	echo "Generating ${GNUPLOT}"
	echo "# autogen" >${GNUPLOT}
	
	echo "set title \"l${LC}r${RC} ${T0S}/${T1S} (${RS})\"" >>${GNUPLOT}
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
	    PLOT="${PLOT}\"${PLOTDATA}\" ${U0} w lp t \"${CTXSTR}\","
	    PLOT="${PLOT}\"${PLOTDATA}\" ${U1} w lp t \"${CTXSTR} (wex)\","
	fi
	echo ${PLOT%\,} >>${GNUPLOT}
	#
        echo "set terminal push" >>${GNUPLOT}
        echo "set terminal postscript eps ${LSR} lw 2 'Helvetica' 10" >>${GNUPLOT}
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