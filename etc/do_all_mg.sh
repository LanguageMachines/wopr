#!/bin/sh
#
TRAINFILE="rmt.1e5"
TESTFILE="rmt.test.1e3"
DEVFILE="rmt.dev.1e3"
FOCUS="rmt.1e5.lex"
#
WOPR="/exp/pberck/wopr/wopr"
CYCLE=0
#
for FFC in 1000 10000 50000
do
  for K in {1..2}
  do
    SCRIPT="do_compare_mg_px.script"
    if test $K -eq 2
    then
      SCRIPT="do_compare_mg_px.script_k2"
    fi
    ID=RUN${CYCLE}.k${K}.ffc${FFC}
    echo "${WOPR} -l -s ${SCRIPT} -p filename:${TRAINFILE},testfile:${TESTFILE},devfile:${DEVFILE},focus:${FOCUS},ffc:${FFC},id:${ID}"
    RES=`${WOPR} -l -s ${SCRIPT} -p filename:${TRAINFILE},testfile:${TESTFILE},devfile:${DEVFILE},focus:${FOCUS},ffc:${FFC},id:${ID} > output.${ID}`
    CYCLE=$(( $CYCLE + 1 ))
  done
done
