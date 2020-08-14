#!/usr/bin/env python3

import subprocess
import sys

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage:", sys.argv[0], "<benchmark>", "<memory limit in GB>")
        sys.exit(1)
    bench = sys.argv[1]
    mem = sys.argv[2]
    a = 'boa'
    d = 'interval'
    if 'SVC' in bench:
        a = 'prover'
        d = 'var-pack-dbm-congruence'

    print('Running IKOS...')
    process = subprocess.Popen(['runexec', '--no-container', '--quiet', '--walltimelimit=3600', '--memlimit=%s000000000' % mem, '--output=/dev/null', '--', 'ikos', '-q', '-w', '--display-time=no', '--display-summary=no', '--progress=no', '--proc=inter', '-a=%s' % a, '-d=%s' % d, bench], stdout=subprocess.PIPE)
    result = process.communicate()[0].decode('utf-8').split('\n')
    for line in result:
        if line.startswith('returnvalue'):
            ret = line.split('=')[1]
            if ret != '0':
                print('Error in IKOS. Return value: ', ret)
                sys.exit(int(ret))
        elif line.startswith('walltime'):
            it = float(line.split('=')[1][:-1])
        elif line.startswith('memory'):
            im = int(line.split('=')[1][:-1])

    print('Running MIKOS...')
    process = subprocess.Popen(['runexec', '--no-container', '--quiet', '--walltimelimit=3600', '--memlimit=%s000000000' % mem, '--output=/dev/null', '--', 'mikos', '-q', '-w', '--display-time=no', '--display-summary=no', '--progress=no', '--proc=inter', '-a=%s' % a, '-d=%s' % d, bench], stdout=subprocess.PIPE)
    result = process.communicate()[0].decode('utf-8').split('\n')
    for line in result:
        if line.startswith('returnvalue'):
            ret = line.split('=')[1]
            if ret != '0':
                print('Error in MIKOS. Return value: ', ret)
                sys.exit(int(ret))
        elif line.startswith('walltime'):
            mt = float(line.split('=')[1][:-1])
        elif line.startswith('memory'):
            mm = int(line.split('=')[1][:-1])

    memratio = mm / im
    speedup = it / mt
    print('-------------------')
    print('IKOS peak memory usage (bytes): ', im)
    print('MIKOS peak memory usage (bytes): ', mm)
    print('Memory reduction ratio (peak memory usage in MIKOS / peak memory usage in IKOS): %.2f' % memratio)
    print('-------------------')
    print('IKOS analysis time (seconds): ', it)
    print('MIKOS analysis time (seconds): ', mt)
    print('Speedup (analysis time in IKOS / analysis time in MIKOS): %.2f' % speedup)
