#!/bin/bash

if [ $# -ne 2 ]; then
  echo "Need argument"
  exit 1
fi

which=$1

cd $2

if [[ ! -d check ]]
then
  mkdir check
fi

for ((i=0; i<200; i++)); do
  curr=$which-$i
  pushd $curr
  pushd *.files
  for f in interval-ikos/*
  do
    bf=$(basename $f)
    echo $bf
    ikos-report -s=full --analyses-filter watch interval-ikos/$bf/ikos-$bf.db > ikos.log
    ikos-report -s=full --analyses-filter watch interval-mikos/$bf/mikos-$bf.db > mikos.log
    echo $bf >> ../../check/$which.txt
    diff ikos.log mikos.log >> ../../check/$which.txt
    if [[ $? -ne 0 ]]; then
      echo $bf >> ../../check/$which.diff.list
    fi
    rm ikos.log mikos.log
  done
  popd
  popd
done
