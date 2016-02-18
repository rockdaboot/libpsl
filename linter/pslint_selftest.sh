#!/bin/sh

rc=0
rm -rf log
mkdir -p log

for file in `ls *.input|cut -d'.' -f1`; do
  echo -n "${file}: "
  ./pslint.py ${file}.input >log/${file}.log 2>&1
  diff ${file}.expected log/${file}.log >log/${file}.diff
  if [ $? -eq 0 ]; then
    echo OK
    rm log/${file}.diff
  else
    echo FAILED
    rc=1
  fi
done

exit $rc
