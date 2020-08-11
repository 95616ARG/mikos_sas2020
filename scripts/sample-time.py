#!/usr/bin/env python3
import subprocess
import time
import os
import glob
import pandas as pd
import datetime
import math
import matplotlib.pyplot as plt
import matplotlib as mpl
import sys
import numpy as np
import scipy.stats
import scipy.stats.mstats

def plot(what):
    df = pd.read_csv(what)
    baset = 'walltime (s)-ikos'
    ourst = 'walltime (s)-mikos'
    basem = 'memory (MB)-ikos'
    oursm = 'memory (MB)-mikos'
    speed = 'speedup'
    mem = 'memreduction'

    df = df[df['status-ikos'].str.startswith('OK')]
    df = df[df['status-mikos'].str.startswith('OK')]
    df = df[df[baset] >= 5]
    df = df[['benchmark', baset, basem, ourst, oursm, mem, speed]]

    dfd = df.sort_values(by=speed, ascending=False)
    dfa = df.sort_values(by=speed, ascending=True)

    if 't1' in what:
        dfd[0:3].to_csv('data/t1-speedup-high.csv', index=False)
        dfa[0:3].to_csv('data/t1-speedup-low.csv', index=False)
        print('Table entries saved in data/.')
    elif 't2' in what:
        dfd[0:3].to_csv('data/t2-speedup-high.csv', index=False)
        dfa[0:3].to_csv('data/t2-speedup-low.csv', index=False)
        print('Table entries saved in data/.')

if __name__ == '__main__':
    plot('data/t1.csv')
    print("---------------------------")
    plot('data/t2.csv')
