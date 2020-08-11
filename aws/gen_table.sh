#!/bin/bash

if [ $# -ne 2 ]; then
  echo "Need argument"
  exit 1
fi

which=$1

cd $2

if [[ ! -d csv ]]
then
  mkdir csv
fi

echo 'benchmark,status-ikos,cputime (s)-ikos,walltime (s)-ikos,memory (MB)-ikos,status-mikos,cputime (s)-mikos,walltime (s)-mikos,memory (MB)-mikos' > csv/$which.csv

for ((i=0; i<200; i++)); do
  curr=$which-$i
  pushd $curr
  # Generate table
  table-generator -f csv *-ikos.xml.bz2 *-mikos.xml.bz2 -n $curr
  # Change tabs to commas
  sed -i 's/\t/,/g' $curr.table.csv
  # Remove headers and test.c
  tail -n +5 $curr.table.csv > $curr.table.csv.new
  # Append to the global csv file
  cat $curr.table.csv.new >> ../csv/$which.csv
  rm $curr.table.csv.new
  popd
done

cd csv
../../ratios.py $which
