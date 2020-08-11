#!/bin/bash
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")

cd $SCRIPT_DIR/..

if [[ ! -d data ]]
then 
  mkdir data
fi

echo 'benchmark,status-ikos,cputime (s)-ikos,walltime (s)-ikos,memory (MB)-ikos,status-mikos,cputime (s)-mikos,walltime (s)-mikos,memory (MB)-mikos' > data/t1.csv

# Generate table
pushd results
table-generator -f csv *t1-ikos.xml.bz2 *t1-mikos.xml.bz2 -n tmp
# Change tabs to commas
sed -i 's/\t/,/g' tmp.table.csv
# Remove headers and test.c
tail -n +5 tmp.table.csv > tmp.table.csv.new
# Append to the global csv file
cat tmp.table.csv.new >> ../data/t1.csv
rm tmp.table.csv.new tmp.table.csv
popd

./scripts/ratios.py data/t1.csv


echo 'benchmark,status-ikos,cputime (s)-ikos,walltime (s)-ikos,memory (MB)-ikos,status-mikos,cputime (s)-mikos,walltime (s)-mikos,memory (MB)-mikos' > data/t2.csv

# Generate table
pushd results
table-generator -f csv *t2-ikos.xml.bz2 *t2-mikos.xml.bz2 -n tmp
# Change tabs to commas
sed -i 's/\t/,/g' tmp.table.csv
# Remove headers and test.c
tail -n +5 tmp.table.csv > tmp.table.csv.new
# Append to the global csv file
cat tmp.table.csv.new >> ../data/t2.csv
rm tmp.table.csv.new tmp.table.csv
popd

./scripts/ratios.py data/t2.csv
