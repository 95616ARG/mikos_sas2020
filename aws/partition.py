#!/usr/bin/env python3

# Partitions 'all.set' and 'tab3.set' into batches in 'batches/'.

import sys
import math

nbatches = 200 # Number of batches.

def line2entry(line):
    return '''    <tasks>
      <include>/home/ubuntu/mikos/benchmarks/%s</include>
    </tasks>''' % line.strip()

if __name__ == '__main__':
    allset = sys.argv[1]

    with open(allset) as f:
        l = f.readlines()
        partition = {}
        for i in range(min(nbatches, len(l))):
            partition[i] = [];
        idx = 0
        for line in l:
            partition[idx].append(line);
            idx = (idx + 1) % nbatches

        with open('%s-template.xml' % allset, 'r') as temp:
            temp = temp.read()
            for i in range(min(nbatches, len(l))):
                tasks = '\n'.join(map(line2entry, partition[i]))
                with open('batches/%s-%d.xml' % (allset, i), 'w') as ff:
                    ff.write(temp.replace('{tasks}', tasks))
