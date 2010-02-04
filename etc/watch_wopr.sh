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
# NB: Does not work on OS X.
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
# Warn if we almost reach $LIMIT
#
WARN=$(echo "scale=0; $LIMIT-($LIMIT/95)" | bc)
WARNED=0
#
while true;
  do
  ps --no-header -p $PID -o etime,pid,rss,vsize,pcpu
  MEM=`ps --no-header -p $PID -o rss`
  if test $WARNED -eq 0 -a $MEM -gt $WARN
  then
    echo "Eeeeh! Limit almost reached"
    if test $MAILTO != ""
    then
      echo "Warning: $PID reached $WARN" | mail -s "Warning limit!" $MAILTO
    fi
    WARNED=1
  fi
  if test $MEM -gt $LIMIT
  then
    echo "$MEM > $LIMIT"
    echo "KILLING PID=$PID"
    kill $PID
    RES=$?
    echo "EXIT CODE=$RES"
    if test $MAILTO != ""
    then
      echo "EXIT CODE=$RES" | mail -s "Killed Wopr" $MAILTO
    fi
    exit
  fi
  sleep 10
done
