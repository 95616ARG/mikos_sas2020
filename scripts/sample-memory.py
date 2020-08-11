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
    # df = pd.read_csv(sys.argv[1])
    df = pd.read_csv(what)
    baset = 'walltime (s)-ikos'
    ourst = 'walltime (s)-mikos'
    basem = 'memory (MB)-ikos'
    oursm = 'memory (MB)-mikos'
    speed = 'speedup'
    mem = 'memreduction'

    # remove errors.
    df = df[~df['status-ikos'].str.startswith('ERROR')]
    df = df[~df['status-mikos'].str.startswith('ERROR')]
    df = df[df['status-ikos'].str.startswith('OK')]
    df = df[df['status-mikos'].str.startswith('OK')]
    df = df[df[baset] >= 5]

    df = df.sort_values(by=mem, ascending=True)
    mdf = df.sort_values(by=basem, ascending=False)

    df = df[['benchmark', baset, basem, ourst, oursm, mem]]
    mdf = mdf[['benchmark', baset, basem, ourst, oursm, mem]]

    if 't1' in what:
        df[0:5].to_csv('data/t1-MRR.csv', index=False)
        mdf[0:5].to_csv('data/t1-MF.csv', index=False)
        # df[0:5].to_latex('oss-top5.tex', index=False)
        # mdf[0:5].to_latex('oss-top5.tex', index=False)
        print('Table entries saved in data/.')
    elif 't2' in what:
        df[0:5].to_csv('data/t2-MRR.csv', index=False)
        mdf[0:5].to_csv('data/t2-MF.csv', index=False)
        print('Table entries saved in data/.')

if __name__ == '__main__':
    plot('data/t1.csv')
    print("---------------------------")
    plot('data/t2.csv')
