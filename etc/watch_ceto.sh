#!/bin/bash
#
# $Id: watch_ceto.sh 6416 2010-10-07 09:56:14Z pberck $
#
# ----
#
# Checks if mem usage on ceto has reached $LIMIT.
# If it has, kills the largest process owned by ${ME} that
# has memory usage over 10%.
# Sends a warning mail if $LIMIT-1 is reached, or when
# disk usages reaches 95%.
# Quits after having killed three processes.
#
# Usage:
# bash ./watch_ceto.sh LIMIT{M|G}
#
# Output:
#  17/90 [1]  (18 %) /exp:36% /exp2:85%  03/26/11 14:54:45
#  |  |        |     |                   |
#  |  |        |     +  disk usage in %  +  time of status
#  |  |        +  % of limit used
#  |  +  % mem limit
#  +  % mem used
#     
# ---- PARAMETERS TO CHANGE ----------------------------------------
#
MAILTO="P.J.Berck@UvT.nl"  # Better change me
DISKS=". /exp /exp2"       # Disks to watch
MINFREE=95                 # Warn when disusage over 95%
ME=`whoami`                # Her processes will be killed
KILLIM=99                  # We quit after having killed 3 processes
SLEEP=10                   # Sleep between cycles
#
# ------------------------------------------------------------------
#
if test $# -lt 1
then
  echo "Supply LIMIT as argument"
  exit
fi
#
LIMIT=$1 # Percentage
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
if test $# -eq 2
then
  SLEEP=$2
fi
#
DATE_CMD="date '+%D %T'"
ECHO="-e"
#
# Warn if we reach $LIMIT-1
#
WARN=$(echo "scale=0; $LIMIT - 1" | bc)
WARNED=0
CYCLE=0
DISKWARNED=0
#
#PREV=`ps -u pberck -o pid,ppid,rss,vsize,pmem | awk '{ rss += $3; vsize += $4; pmem += $5;} ; END { print  pmem }'`
PREV=`ps -eo pid,ppid,rss,vsize,pmem | awk '{ rss += $3; vsize += $4; pmem += $5;} ; END { print  pmem }'`
PREV=${PREV/,/.}  
PREV=${PREV/.*}  
PREV=`echo "$PREV * 1024 * 1024" | bc` #why is this necessary now?
#
echo "Watching; warn:$WARN, limit:$LIMIT"
#
while true;
  do
  #MEM=`ps -u pberck -o pid,ppid,rss,vsize,pmem | awk '{ rss += $3; vsize += $4; pmem += $5;} ; END { print  pmem }'`
  MEM=`ps -eo pid,ppid,rss,vsize,pmem | awk '{ rss += $3; vsize += $4; pmem += $5;} ; END { print  pmem }'`
  MEM=${MEM/,/.}
  MEM=${MEM/.*}
  MEM=`echo "$MEM * 1024 * 1024" | bc` #why is this necessary now?
  PERC=$(echo "scale=0; $MEM * 100 / $LIMIT" | bc) #percentage of limit
  DIFF=$(echo "scale=0; $MEM - $PREV" | bc)
  PREV=$MEM
  NOW=`eval $DATE_CMD`
  #process could disappear before MEM=... and here...
  # Disk space
  #
  DSTR=""
  for MP in ${DISKS}
  do
      USE=`df -kP $MP | tail -n 1 | awk '{print $5}'`
      USE=`echo $USE | sed s/%//`
      if [[ $USE -gt $MINFREE ]]; then
	  #echo "Space critical on $MP:  $USE" 
	  if [[ $DISKWARNED -eq 0 ]]
	  then
	      # Mail a warning, once
	      if test $MAILTO != ""
	      then
		  echo $ECHO "Warning: Diskspace at $MINFREE" | mail -s "Warning limit!" $MAILTO
	      fi
	      DISKWARNED=1
	  fi
	  DSTR="${DSTR} *${MP}:${USE}%* "
      else
	  DSTR="${DSTR}${MP}:${USE}% "
      fi
  done
  #
  echo $ECHO "\r\033[K    \c"
  echo $ECHO "\r $MEM/$LIMIT [$DIFF]  ($PERC %) $DSTR $NOW \c"
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
	  BIGGEST=`ps -u ${ME} -o pid,ppid,rss,vsize,pmem | sort -n -k5 | tail -n1 | awk '{print $1}'`
	  BIGMEM=`ps -u ${ME} -o pid,ppid,rss,vsize,pmem | sort -n -k5 | tail\
 -n1 | awk '{print $5}'`
	  BIGMEM=${BIGMEM/.*}
	  if [[ $BIGMEM -gt 10 ]] #...could be many small woprs...
	  then
	      ps -u ${ME} -o pid,rss,vsize,pmem,cmd | sort -n -k5 | tail -n5
	      echo $BIGGEST
	      echo $ECHO "KILLING PID=$BIGGEST"
	      kill $BIGGEST
	      RES=$?
	      echo "EXIT CODE=$RES"
	      if test $MAILTO != ""
	      then
		  echo "EXIT CODE=$RES\nKILL LIM=$KILLIM" | mail -s "Killed Wopr (${BIGGEST})" $MAILTO
	      fi
	      KILLIM=$(( $KILLIM - 1 ))
	      if [[ $KILLIM -eq 0 ]]
	      then
		  echo "No more kills left"
		  exit 
	      fi
	  fi
      fi
  fi
  #
  sleep $SLEEP
  CYCLE=$(( $CYCLE + 1 ))
done
