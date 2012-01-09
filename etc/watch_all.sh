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
LIMIT=64000000
SLEEP=10
DATE_CMD="date '+%s'"
ECHO="-e"
PS_MEM="ps -eo rss,comm,user | grep e | grep pberck | awk '{ SUM += $1} END { print SUM/1024/1024 }'"
#
while true;
  do
  #ps --no-header -p $PID -o etime,pid,rss,vsize,pcpu
  MEM=`eval ps -eo rss,comm,user | grep e | grep pberck | awk '{ SUM += $1} END { print SUM }'`
  #
  echo $ECHO "\r                       \c"
  echo $ECHO "\rWoprs: $MEM \c"
  if test $MEM -gt $LIMIT
  then
    echo $ECHO "\n$MEM > $LIMIT"
    if test $MAILTO != ""
    then
      echo "Warning memory" | mail -s "Too many woprs" $MAILTO
    fi
    exit
  fi
  #
  sleep $SLEEP
done
