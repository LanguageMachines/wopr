#!/bin/sh
#
# $Id$
#
# ----
# Script to check every ten second if process with $PID has an RSS
# size larger than $LIMIT kB. If so, kill it and send me an email.
#
# Usage:
# sh ./watch_wopr.sh PID SIZE
#
# NB: doesn't check for ownership, name of process &c.
# ----
#
#
MAILTO="P.J.Berck@UvT.nl"
#
if test $# -lt 2
then
  echo "Supply PID and MEMSIZE as arguments."
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
MEM_FILE=$PID"_mem.txt"
SLEEP=10
MEMWRITE_INTERVAL=60
MEMWRITE_LAST=0;
DATE_CMD="date '+%s'"
#
PS_MEM="ps --no-header -p $PID -o rss"
PS_TIME="ps --no-header -p $PID -o etime"
PS_MEMFILE="ps --no-header -p $PID -o etime,rss,vsize,pcpu"
ECHO="-e"
MACH=`uname`
if test $MACH == "Darwin"
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
      echo "Process gone." | mail -s "Process gone." $MAILTO
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
