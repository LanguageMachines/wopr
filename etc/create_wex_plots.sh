#!/bin/sh
#
# Script to generate gnuplot files from the data file generated
# by do_meta_nyt.sh.
#
PREFIX="pre"
VAR="cg"
if test $# -lt 1
then
  echo "Supply DATAFILE PREFIX PLOTVAR"
  exit
fi
#
DATA=$1
PREFIX=$2
PLOTVAR=$3
#
U0="using 2:7"
U1="using 2:11"
XR="[9000:]"
YR="[]"
if [[ $PLOTVAR == "cd" ]]
then
    U0="using 2:8"
    U1="using 2:12"  
fi
if [[ $PLOTVAR == "ic" ]]
then
    U0="using 2:9"
    U1="using 2:13"  
fi
if [[ $PLOTVAR == "pplx" ]]
then
    U0="using 2:20"
    U1="using 2:21"  
fi
#
# Determine algos used from file.
# Determine the LC and RC used.
# Format:
# WEX10321 500000 l3r1 2 1 cg 65433 68482 98912 0.69 66377 64408 102042 0.78 -a1+D -a4+D 1-500
#
# We have newer files with 4 extra fields appended per line, with 
# perplexity (px1, mg1, px2, mg2):
# WEX10230 5000000 l2r0 1 1 cg 21.41 51.11 27.46 0.41 21.41 51.11 27.46 0.41 -a4+D -a4+D 1-200 196.12 196.12 174.35 174.35
#
# extra "var to plot" parameter? The U0 and U1 later on are hardcoded now.
#
LCS=`cut -d' ' -f 3 ${DATA}  | cut -c2 | sort -u`
RCS=`cut -d' ' -f 3 ${DATA}  | cut -c4 | sort -u`
T0S=`cut -d' ' -f 15 ${DATA}  | sort -u`
T1S=`cut -d' ' -f 16 ${DATA}  | sort -u`
RS=`cut -d' ' -f 17 ${DATA}  | sort -u`
VAR=`cut -d' ' -f 6 ${DATA}  | sort -u` #optimalisation variable!
echo $T1S
echo $VAR
#
# First, get the data from all the experiments in their
# own file.
#
for LC in ${LCS}
do
    for RC in ${RCS}
    do
	CTXSTR=l${LC}r${RC}
	PLOTDATA=${PREFIX}_${CTXSTR}_${VAR}_${PLOTVAR}.data
	echo "Generating ${PLOTDATA}"
	egrep " ${CTXSTR} (.*?) (.?*) ${VAR}" ${DATA} > ${PLOTDATA}
    done
done
#
# File per context size
#
for LC in ${LCS}
do
    for RC in ${RCS}
    do
	CTXSTR=l${LC}r${RC}_${VAR}_${PLOTVAR}
	GNUPLOT=${PREFIX}_${CTXSTR}.plot
	PSPLOT=${PREFIX}_${CTXSTR}.ps
	echo "Generating ${GNUPLOT}"
	echo "# autogen" >${GNUPLOT}
	
	echo "set title \"l${LC}r${RC} ${T0S}/${T1S} ${VAR} (${RS})\"" >>${GNUPLOT}
	echo "set xlabel \"instances\""  >>${GNUPLOT}
	echo "set key bottom"  >>${GNUPLOT}
	echo "set logscale x" >>${GNUPLOT}
	echo "set ylabel \"${PLOTVAR}\""  >>${GNUPLOT}
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