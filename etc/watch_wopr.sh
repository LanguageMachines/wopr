#!/bin/sh
#
# $Id$
#
# ----
# Script to check every ten second if process with $PID has an RSS
# size larger than $LIMIT kB. If so, kill it and send me an email.
#
# NB: doesn't check for ownership, name of process &c.
# ----
#
LIMIT=103809024 # 99 GB
PID=$$          # Set this to the right process ID
#
while true;
  do
  ps --no-header -p $PID -o etime,pid,rss,vsize,pcpu
  MEM=`ps --no-header -p $PID -o rss`
  if test $MEM -gt $LIMIT
  then
    echo "$MEM > $LIMIT"
    echo "KILLING PID=$PID"
    kill $PID
    RES=$?
    echo "EXIT CODE=$RES"
#    echo "EXIT CODE=$RES" | mail -s "Killed Wopr" p.j.berck@uvt.nl
    exit
  fi
  sleep 10
done
