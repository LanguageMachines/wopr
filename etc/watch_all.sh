#!/bin/sh
#
# $Id: watch_all.sh 6416 2010-10-07 09:56:14Z pberck $
#
# ----
# Chekcs size of all woprs
# ----
#
#
MAILTO="P.J.Berck@UvT.nl"
#
LIMIT=99 #GB
SLEEP=10
#
if test $# -lt 1
then
  echo "Supply LIMIT in GB as argument."
  exit
fi
#
LIMIT=$1 
LIMIT=`echo "$LIMIT * 1024 * 1024" | bc` #kB
#
DATE_CMD="date '+%s'"
ECHO="-e"
#
# Greps my woprs
PS_MEM="ps -eo rss,comm,user | grep wopr | grep pberck | awk '{ SUM += $1} END { print SUM/1024/1024 }'"
#
while true;
  do
  #ps --no-header -p $PID -o etime,pid,rss,vsize,pcpu
  MEM=`eval ps -eo rss,comm,user | grep wopr | grep pberck | awk '{ SUM += $1} END { print SUM }'`
  NUM=`ps -eo rss,comm,user | grep wopr | grep pberck | wc -l`
  PERC=$(echo "scale=1; $MEM * 100 / $LIMIT" | bc)
  #
  echo $ECHO "\r                                           \c"
  echo $ECHO "\rWoprs: $MEM ($NUM wopr(s), $PERC% of limit)\c"
  if test $MEM -gt $LIMIT
  then
    echo $ECHO "\n$MEM > $LIMIT"
    if test $MAILTO != ""
    then
      echo "Warning memory: ${MEM}" | mail -s "Too many woprs" $MAILTO
    fi
    #exit
    LIMIT=`echo "$LIMIT + 1024000" | bc`
    echo "Limit now: ${LIMIT}"
  fi
  #
  sleep $SLEEP
done
