#!/bin/sh
#
# $Id$
#
# ----
# Script to check every n second if process with $PID has an RSS
# size larger than $LIMIT kB. If so, kill it and send me an email.
# Gives a warning when 95% of memory limit has been reached.
# Kills the process when it goes over 100%.
#
# Usage:
# sh ./watch_wopr.sh PID SIZE{M|G} [SLEEP [MEMWRITE_INTERVAL]]
#
# Output:
# durian:wopr pberck$ sh etc/watch_wopr.sh 12019 2M
# Watching pid:12038, warn:2027, limit:2048
# 12038:   1076/2048 [0]  (52.5 %)  11:20    
# ^PID     ^MEM ^     ^%DIFF  ^     ^time running
#               | LIMIT       | % used
#
# NB: doesn't check for ownership, name of process &c. Size is in kB
# unles suffixed by M or G, then it is multiplied by 1024 or 1024^2 
# respectively. Default SLEEP is 10 seconds, MEMWRITE_INTERVAL is 60.
#
# TODO: time of CPU limits.
#
# ----
#
#
MAILTO="P.J.Berck@UvT.nl"
#
if test $# -lt 2
then
  echo "Supply PID and MEMSIZE as arguments, optional SLEEP and WRITEINTERVAL"
  exit
fi
#
PID=$1   # The process ID
LIMIT=$2 # kB
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
MEMWRITE_INTERVAL=60
#
if test $# -eq 3
then
  SLEEP=$3
fi
if test $# -eq 4
then
  SLEEP=$3
  MEMWRITE_INTERVAL=$4
fi
#
MEM_FILE=$PID"_mem.txt"
MEMWRITE_LAST=0;
DATE_CMD="date '+%s'"
#
# Create plot file
#
PLOTFILE=$PID"_mem.plot"
PLOTOUT=$PID"_mem.svg"
echo "set terminal svg dynamic fsize 10" > $PLOTFILE
echo "set output \"$PLOTOUT\"" >> $PLOTFILE
echo "set xdata time" >> $PLOTFILE
echo "set timefmt \"%H:%M:%S\"" >> $PLOTFILE
echo "set format y \"%10.0f\"" >> $PLOTFILE
echo "plot \"$PLOTFILE\" using 1:3 with lines,\\" >> $PLOTFILE
echo "\"$PLOTFILE\" using 1:4 with lines,\\" >> $PLOTFILE
echo "\"$PLOTFILE\" using 1:((\$4)-(\$3))" >> $PLOTFILE
#
# --
#
PS_MEM="ps --no-header -p $PID -o rss"
PS_TIME="ps --no-header -p $PID -o etime"
PS_MEMFILE="ps --no-header -p $PID -o etime,rss,vsize,pcpu"
ECHO="-e"
MACH=`uname`
if test $MACH == "Darwin" # OS X definitions
then
  PS_MEM="ps -p $PID -o rss=''"
  PS_TIME="ps -p $PID -o etime=''"
  PS_MEMFILE="ps -p $PID -o etime='',rss='',vsize='',pcpu=''"
  ECHO=""
fi
#
# Warn if we almost reach $LIMIT
#
WARN=$(echo "scale=0; $LIMIT-($LIMIT/95)" | bc)
WARNED=0
CYCLE=0
#
PREV=`eval $PS_MEM`
#
echo "Watching pid:$PID, warn:$WARN, limit:$LIMIT"
#
while true;
  do
  #ps --no-header -p $PID -o etime,pid,rss,vsize,pcpu
  MEM=`eval $PS_MEM`
  # Test if length of MEM is < 1
  if test ${#MEM} -lt 1
  then
    echo $ECHO "\nProcess gone."
    if test $MAILTO != ""
    then
      echo "Process $PID gone." | mail -s "Process $PID gone." $MAILTO
    fi
    exit
  fi
  PERC=$(echo "scale=1; $MEM * 100 / $LIMIT" | bc)
  DIFF=$(( $MEM - $PREV ))
  PREV=$MEM
  TIME=`eval $PS_TIME`
  #process could disappear before MEM=... and here...
  echo $ECHO "\r                                                             \c"
  echo $ECHO "\r$PID: $MEM/$LIMIT [$DIFF]  ($PERC %)  $TIME\c"
  if test $WARNED -eq 0 -a $MEM -gt $WARN
  then
    echo $ECHO "\nEeeeh! Limit almost reached"
    if test $MAILTO != ""
    then
      echo $ECHO "Warning: $PID reached $WARN" | mail -s "Warning limit!" $MAILTO
    fi
    WARNED=1
    CYCLE=0 #skip direct kill
  fi
  # We don't kill before we finished one cycle. If you specify
  # too little memory you might have time to hit ctrl-C.
  if test $MEM -gt $LIMIT -a $CYCLE -gt 0
  then
    echo $ECHO "\n$MEM > $LIMIT"
    echo $ECHO "KILLING PID=$PID"
    kill $PID
    RES=$?
    echo "EXIT CODE=$RES"
    if test $MAILTO != ""
    then
      echo "EXIT CODE=$RES" | mail -s "Killed Wopr" $MAILTO
    fi
    exit
  fi
  #
  # If time > memwrite_interval, write mem to file.
  #
  CUR_DATE=`eval $DATE_CMD`
  DATE_DIFF=$(( $CUR_DATE - $MEMWRITE_LAST ))
  if test $DATE_DIFF -ge $MEMWRITE_INTERVAL
  then
    #echo "  $MEMWRITE_LAST"
    echo `eval $PS_MEMFILE` >> $MEM_FILE
    MEMWRITE_LAST=$CUR_DATE
  fi
  #
  sleep $SLEEP
  CYCLE=$(( $CYCLE + 1 ))
done
