#!/bin/bash
#
while getopts d:s:e: o
do case "$o" in
	s)      START="$OPTARG";;
	e)      END="$OPTARG";;
	d)      DATA="$OPTARG";;
	[?])    echo "Usage: $0 [-s start] [-e end]"
	        exit 1;;
    esac
done
#
if [[ $DATA ]]
then
    L1=`awk '{print $1}' ${DATA} | sed 's/...\(.*\)/\1/' | sort -n | tail -n1`
    L0=`awk '{print $1}' ${DATA} | sed 's/...\(.*\)/\1/' | sort -n | head -n1`
    START=${L0}
    END=${L1}
    echo "$L0 - $L1"
else
    DATA=DATA.wex.${START}.data
fi
DATANEW=${DATA}.pplx
PPLX="/exp/pberck/wopr/etc/pplx_px.pl"
#
for W in $(seq ${START} 1 ${END})
do
  OUTFILE="output.WEX"${W};
  echo $OUTFILE;
  #
  # NB:
  # We grab pplx figures twice, first for the unfiltered run, and then
  # for the filtered run.
  #
  if [ -f ${OUTFILE} ]
  then
      # First look up the invocation string to get filename etc.
      #
      LINE=`grep compare_mg ${OUTFILE} | head -n1`
      #
      #18:11:09.72: EXTERN: perl /exp/pberck/wopr/etc/compare_mg_px.pl -p nyt.tail20000b.l3r0_WEX11408a.px -m nyt.tail20000b.l3r0_WEX11408a.mg -l 3 -r 0 | grep TOTAL
      #
      # Grab the filenames and context size.
      #
      RX='.* \-p (.*) \-m (.*) \-l (.*) \-r (.*) \|.*'
      if [[ "${LINE}" =~ $RX ]]
      then
	  px1file=${BASH_REMATCH[1]}
	  mg1file=${BASH_REMATCH[2]}
	  lc=${BASH_REMATCH[3]}
	  rc=${BASH_REMATCH[4]}
	  #
	  LINE=`perl ${PPLX} -f ${px1file} -l ${lc} -r ${rc} | tail -n10 | grep ppl`
	  #
	  #Wopr ppl:    373.66 Wopr ppl1:    373.66  (No oov words.)
	  #
	  # Grab the perplexity from the output.
	  #
	  RX='.* ([0-9]+\.[0-9]+) .* ([0-9]+\.[0-9]+) .*'
	  px1ppl=0
	  px1ppl1=0
	  if [[ "${LINE}" =~ $RX ]]
	  then
	      px1ppl=${BASH_REMATCH[1]}
	      px1ppl1=${BASH_REMATCH[2]}
	  else
	      echo "NO MATCH"
	  fi
	  # And likewise for the mg file.
	  #
	  LINE=`perl ${PPLX} -f ${mg1file} -l ${lc} -r ${rc} | tail -n10 | grep ppl`
	  #
	  RX='.* ([0-9]+\.[0-9]+) .* ([0-9]+\.[0-9]+) .*'
	  mg1ppl=0
	  mg1ppl1=0
	  if [[ "${LINE}" =~ $RX ]]
	  then
	      mg1ppl=${BASH_REMATCH[1]}
	      mg1ppl1=${BASH_REMATCH[2]}
	      #echo ${OUTFILE} ${px1ppl} ${mg1ppl}
	  else
	      echo "NO MATCH"
	  fi
      else
	  echo "NO MATCH"
      fi
      #
      # Repeat for the second run.
      #
      LINE=`grep compare_mg ${OUTFILE} | tail -n1`
      #
      # Grab the filenames and context size.
      #
      RX='.* \-p (.*) \-m (.*) \-l (.*) \-r (.*) \|.*'
      if [[ "${LINE}" =~ $RX ]]
      then
	  px2file=${BASH_REMATCH[1]}
	  mg2file=${BASH_REMATCH[2]}
	  lc=${BASH_REMATCH[3]}
	  rc=${BASH_REMATCH[4]}
	  #
	  LINE=`perl ${PPLX} -f ${px2file} -l ${lc} -r ${rc} | tail -n10 | grep ppl`
	  #
	  # Grab the perplexity from the output.
	  #
	  RX='.* ([0-9]+\.[0-9]+) .* ([0-9]+\.[0-9]+) .*'
	  px2ppl=0
	  px2ppl1=0
	  if [[ "${LINE}" =~ $RX ]]
	  then
	      px2ppl=${BASH_REMATCH[1]}
	      px2ppl1=${BASH_REMATCH[2]}
	  else
	      echo "NO MATCH"
	  fi
	  # And likewise for the mg file.
	  #
	  LINE=`perl ${PPLX} -f ${mg2file} -l ${lc} -r ${rc} | tail -n10 | grep ppl`
	  #
	  RX='.* ([0-9]+\.[0-9]+) .* ([0-9]+\.[0-9]+) .*'
	  mg2ppl=0
	  mg2ppl1=0
	  if [[ "${LINE}" =~ $RX ]]
	  then
	      mg2ppl=${BASH_REMATCH[1]}
	      mg2ppl1=${BASH_REMATCH[2]}
	      #echo ${OUTFILE} ${px1ppl} ${mg1ppl} ${px2ppl} ${mg2ppl}
	      #
	      DATALINE=`grep WEX${W} ${DATA} | tail -n1`
	      echo $DATALINE ${px1ppl} ${mg1ppl} ${px2ppl} ${mg2ppl}
	      echo $DATALINE ${px1ppl} ${mg1ppl} ${px2ppl} ${mg2ppl} >> ${DATANEW}
	  else
	      echo "NO MATCH"
	  fi
      else
	  echo "NO MATCH"
      fi
  fi
done
