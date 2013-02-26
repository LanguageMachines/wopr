#!/bin/bash
#
shopt -s nullglob
#
PORT=2000
HOST="localhost"
PYFILE="py.out"
TSFILE="ts.out"
DFLT=0
#
echo "port=${PORT}" > ${TSFILE}
echo "maxconn=2" >> ${TSFILE}
#
echo "s1=TServers(\"${HOST}\",${PORT})" > ${PYFILE}
ALLC="allc=[ "
#
for FILE in *ibase
do
    #echo ${FILE}
    #austen.train.l2r2.c0035
    #austen.train.l2r2.c0000_-a4+D.ibase
    RX='(.*)l(.)r(.)\.c(....)_(.*)(\+.*).ibase'
    if [[ "${FILE}" =~ $RX ]]
    then
	BASE=${BASH_REMATCH[1]}
	LC=${BASH_REMATCH[2]}
	RC=${BASH_REMATCH[3]}
	C=${BASH_REMATCH[4]}
	T0=${BASH_REMATCH[5]}
	T1=${BASH_REMATCH[6]}
	#ts0010=-i austen.train.l2r2.c0011_-a4+D.ibase -a4 +D +vdb+di
	TS="ts${C}=\"-i ${FILE} ${T0} ${T1} +vdb+di\""
	PY="ts${C}=Classifier(\"ts${C}\", ${LC}, ${RC}, s1)"
	#
	RX='(.*\.c....)_-(.*).ibase'
	if [[ "${FILE}" =~ $RX ]]
	then
	    BASE=${BASH_REMATCH[1]}
	    #echo '{print $NF}' ${BASE} |sort -u
	    CS=`awk '{print $NF}' ${BASE} |sort -u|tr '\n' ' '`
	    echo "#${BASE}:${CS}" >> ${PYFILE}
	fi
	#
	echo ${TS} >> ${TSFILE}
	echo ${PY} >> ${PYFILE}
	ALLC="${ALLC}ts${C}, "
    fi
    #
    #dflt=-i austen.train.l2r2_-a4+D.ibase -a4 +D +vdb+di
    RX='(.*)l(.)r(.)_(.*)(\+.*).ibase'
    if [[ "${FILE}" =~ $RX ]]
    then
	BASE=${BASH_REMATCH[1]}
	LC=${BASH_REMATCH[2]}
	RC=${BASH_REMATCH[3]}
	T0=${BASH_REMATCH[4]}
	T1=${BASH_REMATCH[5]}
	TS="dflt${DFLT}=\"-i ${FILE} ${T0} ${T1} +vdb+di\""
	PY="dflt${DFLT}=Classifier(\"dflt${DFLT}\", ${LC}, ${RC}, s1)"
	echo ${TS} >> ${TSFILE}
	echo ${PY} >> ${PYFILE}
	ALLC="${ALLC} dflt${DFLT}, "
	DFLT=$(( $DFLT + 1 ))
    fi
done
#awk  '{print $NF}' bla.c0001 |sort -u
ALLC=${ALLC%%??} #removes last 2 chars
ALLC="${ALLC} ]"
echo ${ALLC} >> ${PYFILE}
