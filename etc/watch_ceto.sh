#!/bin/sh
#
# $Id: watch_ceto.sh 6416 2010-10-07 09:56:14Z pberck $
#
# ----
#
# Checks if mem usage on ceto has reached $LIMIT.
# If it has, kills the largest wopr owned by pberck.
#
# Usage:
# sh ./watch_ceto.sh LIMIT{M|G}
#
# ----
#
#
MAILTO="P.J.Berck@UvT.nl"
#
if test $# -lt 1
then
  echo "Supply LIMIT as argument"
  exit
fi
#
LIMIT=$1 # kB
#
PRE=`echo $LIMIT | sed s/.$//`
SUF=`echo $LIMIT | sed -e "s/^.*\(.\)$/\1/"`
if test $SUF == 'M'
then
  LIMIT=`echo "$PRE * 1024" | bc`
fi
if test $SUF == 'G'
then
  LIMIT=`echo "$PRE * 1024 * 1024" | bc`
fi
#
SLEEP=10
if test $# -eq 3
then
  SLEEP=$3
fi
#
MEM_FILE="ceto_mem.txt"
MEMWRITE_LAST=0;
DATE_CMD="date '+%D %T'"
#
# --
#
ECHO="-e"
MACH=`uname`
if test $MACH == "Darwinddd" # OS X definitions
then
  ECHO=""
fi
#
# Warn if we almost reach $LIMIT
#
WARN=$(echo "scale=0; $LIMIT - 1" | bc)
WARNED=0
CYCLE=0
#
#PREV=`ps -u pberck -o pid,ppid,rss,vsize,pmem | awk '{ rss += $3; vsize += $4; pmem += $5;} ; END { print  pmem }'`
PREV=`ps -eo pid,ppid,rss,vsize,pmem | awk '{ rss += $3; vsize += $4; pmem += $5;} ; END { print  pmem }'`
PREV=${PREV/,/.}  
PREV=${PREV/.*}  
#
echo "Watching; warn:$WARN, limit:$LIMIT"
#
while true;
  do
  #MEM=`ps -u pberck -o pid,ppid,rss,vsize,pmem | awk '{ rss += $3; vsize += $4; pmem += $5;} ; END { print  pmem }'`
  MEM=`ps -eo pid,ppid,rss,vsize,pmem | awk '{ rss += $3; vsize += $4; pmem += $5;} ; END { print  pmem }'`
  MEM=${MEM/,/.}
  MEM=${MEM/.*}
  PERC=$(echo "scale=0; $MEM * 100 / $LIMIT" | bc) #percentage of limit
  DIFF=$(echo "scale=0; $MEM - $PREV" | bc)
  PREV=$MEM
  NOW=`eval $DATE_CMD`
  #process could disappear before MEM=... and here...
  echo $ECHO "\r                                                           \c"
  echo $ECHO "\r $MEM/$LIMIT [$DIFF]  ($PERC %) $NOW\c"
  if [[ $WARNED -eq 0 ]]
  then
      if [[ $MEM -gt $WARN ]]
      then
	  echo $ECHO "\nEeeeh! Limit almost reached"
	  if test $MAILTO != ""
	  then
	      echo $ECHO "Warning: ceto at $WARN" | mail -s "Warning limit!" $MAILTO
	  fi
	  WARNED=1
	  CYCLE=0 #skip direct kill
      fi
  fi
  # We don't kill before we finished one cycle. If you specify
  # too little memory you might have time to hit ctrl-C.
  if [[ $MEM -gt $LIMIT ]]
      then
      if [[ $CYCLE -gt 0 ]]
      then
	  echo $ECHO "\n$MEM > $LIMIT"
	  BIGGEST=`ps -u pberck -o pid,ppid,rss,vsize,pmem | sort -n -k5 | tail -n1 | awk '{print $1}'`
	  echo $BIGGEST
	  echo $ECHO "KILLING PID=$BIGGEST"
	  kill $BIGGEST
	  RES=$?
	  echo "EXIT CODE=$RES"
	  if test $MAILTO != ""
	  then
	      echo "EXIT CODE=$RES" | mail -s "Killed Wopr (${BIGGEST})" $MAILTO
	  fi
	  exit
      fi
  fi
  #
  sleep $SLEEP
  CYCLE=$(( $CYCLE + 1 ))
done
