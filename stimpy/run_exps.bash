#!/bin/bash
#
# Create data, create a wopr.script. Runs wopr.script which
# creates the instance bases.
# The dir_to_tspy.bash script creates two files,
# ts.out which is a timblserver config, and
# py.out which is a Python definition to use in a
# python script (stimpy).
#
# Example:
# bash run_exps.bash -r 2 -C 200
# bash run_exps.bash -l 3 -C 300
# after these two, ts.out and py.out will contain
# all classifiers created.
#
ID="0000"
TRAINFILE="austen.train"
TESTFILE="austen.test"
WOPR="/users/pberck/uvt/wopr/wopr"
TSERVER="timblserver"
TIMBL="-a1 +D"
LC=2
RC=0
C=0
CLIST="clist"
CONFUSED="confusedata.py"
WSCRIPT="wopr.script"
#
while getopts “c:C:h:p:i:l:r:t:T:” OPTION
do
    case $OPTION in
        c)
            CLIST=${OPTARG}
            ;;
        C)
            C=${OPTARG}
            ;;
        h)
            HOST=${OPTARG}
            ;;
        p)
            PORT=${OPTARG}
            ;;
        i)
            ID=${OPTARG}
            ;;
        l)
            LC=${OPTARG}
            ;;
        r)
            RC=${OPTARG}
            ;;
        t)
            TRAINFILE=${OPTARG}
            ;;
        T)
            TIMBL=${OPTARG}
            ;;
	*)
            echo "Unknown error while processing options"
            ;;
    esac
done
#
BLOB=`${WOPR} | head -n1` #09:30:51.01: Starting wopr 1.35.5
#http://www.arachnoid.com/linux/shell_programming.html
STR=${BLOB#*wopr }
RX='([0123456789.]*)\.([0123456789.]*)'
if [[ "$STR" =~ $RX ]]
then
    WV=${BASH_REMATCH[1]}
    #echo ${WV}
    if [[ "${WV}" < "1.35" ]]
    then
	echo "NO UPTODATE WOPR VERSION."
	exit 1
    fi
fi
#
#Make the dataset here, clean up afterwards?
if [ ! -e "${TRAINFILE}" ]
then
    echo "Data file ${TRAINFILE} not found."
    exit 2
fi
if [ ! -e "${TESTFILE}" ]
then
    echo "Data file ${TESTFILE} not found."
    exit 2
fi
#
FREE=`df -k . | tail -1 | awk '{print $3}'`
# 25 577 264 = 25GB
if [[ ${FREE} -lt 100000 ]]
then  
    echo "Space critical, exiting."
    exit 3
fi
#
echo "Creating lexicon windowed data sets."
echo ${WOPR} -r lexicon,window_lr -p filename:${TRAINFILE},rc:${RC},lc:${LC}
${WOPR} -r lexicon,window_lr -p filename:${TRAINFILE},rc:${RC},lc:${LC} > output.${ID}
TRAINFILE=${TRAINFILE}.l${LC}r${RC}
echo ${WOPR} -r window_lr -p filename:${TESTFILE},rc:${RC},lc:${LC}
${WOPR} -r window_lr -p filename:${TESTFILE},rc:${RC},lc:${LC} >> output.${ID}
TESTFILE=${TESTNFILE}.l${LC}r${RC}
LEXFILE=${TRAINFILE}.lex
#
# python confusedata.py -c clist -f austen.train.l2r2 >wopr.script
# confusdata.py takes windowed files, and selects subsets based
# on the ${CLIST}.
#
echo python ${CONFUSED} -c ${CLIST} -f ${TRAINFILE} --lc=${LC} --rc=${RC} --count=${C} to ${WSCRIPT}
python ${CONFUSED} -c ${CLIST} -f ${TRAINFILE} --lc=${LC} --rc=${RC} --count=${C} > ${WSCRIPT}
#
echo ${WOPR} -s ${WSCRIPT} -p timbl:"${TIMBL}"
${WOPR} -s ${WSCRIPT} -p timbl:"${TIMBL}" >> output.${ID}
#
echo "bash dir_to_tspy.bash"
bash dir_to_tspy.bash 
