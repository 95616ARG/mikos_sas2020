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

    df = df[df['status-ikos'].str.startswith('OK')]
    df = df[df['status-mikos'].str.startswith('OK')]
    dfff = df[df[baset] >= 5]

    ax = plt.subplot(111)
    ax.set_xscale('log')
    ax.set_yscale('log')

    plt.scatter(dfff[baset].values, dfff[ourst].values, c='green', marker='+', alpha=0.8)

    plt.rc('text', usetex=True)
    plt.grid(b=True, ls='--', alpha=0.3)
    plt.xlabel(r'Runtime of $\textsc{ikos}$ (seconds)')
    plt.ylabel(r'Runtime of $\textsc{Mikos}$ (seconds)')

    mins = dfff.loc[dfff[speed].idxmin()]
    maxs = dfff.loc[dfff[speed].idxmax()]

    mx = max(dfff[baset].max(), dfff[ourst].max()) + 1000
    mn = max(min(dfff[baset].min(), dfff[ourst].min()) - 10, 1)

    plt.xlim(mn, mx)
    plt.ylim(mn, mx)

    lims = [mn,mx]
    lims2 = [mn/2.0,mx/2.0]
    limsh = [2.0*mn,2.0*mx]

    ax.plot(lims, lims, 'k-', alpha=1, zorder=0, label="$y = x$")
    ax.plot(lims, limsh, 'k-.', alpha=1, zorder=0, label="$y = 2 x$")
    ax.plot(lims, lims2, 'k:', alpha=1, zorder=0, label="$y = 1/2 x$")

    plt.legend()

    print('Min, Max: ', dfff[speed].min(), dfff[speed].max())

    gm = scipy.stats.mstats.gmean(dfff[speed].values)
    print('Geometric mean: ', gm)

    if 't1' in what:
        plt.savefig('data/scat-time-t1.png', bbox_inches='tight')
        plt.clf()
        print('\nFigure saved to "data/scat-time-t1.png".')
    elif 't2' in what:
        plt.savefig('data/scat-time-t2.png', bbox_inches='tight')
        plt.clf()
        print('\nFigure saved to "data/scat-time-t2.png".')

if __name__ == '__main__':
    mpl.rcParams['text.usetex'] = True
    mpl.rcParams['text.latex.preamble'] = [r'\usepackage{amsmath}']
    mpl.rcParams['font.size'] = 17.0
    mpl.rcParams['axes.labelsize'] = 20.0
    mpl.rcParams['ytick.labelsize'] = 20.0
    mpl.rcParams['xtick.labelsize'] = 20.0
    mpl.rcParams['font.weight'] = 'bold'

    plot('data/t1.csv')
    print("---------------------------")
    plot('data/t2.csv')
