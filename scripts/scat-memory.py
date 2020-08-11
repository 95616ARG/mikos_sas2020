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
    mem = 'memreduction'

    # remove errors and filter out less than 5 seconds in IKOS.
    df = df[~df['status-ikos'].str.startswith('ERROR')]
    df = df[~df['status-mikos'].str.startswith('ERROR')]
    df = df[df['status-mikos'].str.startswith('OK')]
    df = df[df[baset] >= 5]

    ax = plt.subplot(111)
    ax.set_xscale('log')
    ax.set_yscale('log')

    dfa = df[df['status-ikos'].str.startswith('OK')]
    print("OK in IKOS also: ", dfa.shape[0])
    dfb = df[~df['status-ikos'].str.startswith('OK')]
    print("NOT OK in IKOS: ", dfb.shape[0])
    plt.scatter(dfa[basem].values, dfa[oursm].values, c='blue', marker='.', alpha=0.8)
    plt.scatter(dfb[basem].values, dfb[oursm].values, c='red', marker='x', alpha=0.8)

    plt.rc('text', usetex=True)
    plt.grid(b=True, ls='--', alpha=0.3)
    plt.xlabel(r'Memory footprint of $\textsc{ikos}$ (MB)')
    plt.ylabel(r'Memory footprint of $\textsc{Mikos}$ (MB)')

    mins = df.loc[df[mem].idxmin()]
    maxs = df.loc[df[mem].idxmax()]
    if 'oss' in what:
        plt.annotate("%.3f" % round(maxs[mem], 3),
                xy=(maxs[basem],maxs[oursm]),
                xytext=(0.55, 0.65),
                textcoords='axes fraction', arrowprops=dict(facecolor='black', shrink=0.05), horizontalalignment='right', verticalalignment='bottom')

        plt.annotate("%.3f" % round(mins[mem], 3),
                xy=(mins[basem],mins[oursm]),
                xytext=(0.98, 0.08),
                textcoords='axes fraction', arrowprops=dict(facecolor='black', shrink=0.05), horizontalalignment='right', verticalalignment='bottom')
    else:
        plt.annotate("%.3f" % round(maxs[mem], 3),
                xy=(maxs[basem],maxs[oursm]),
                xytext=(0.25, 0.35),
                textcoords='axes fraction', arrowprops=dict(facecolor='black', shrink=0.05), horizontalalignment='right', verticalalignment='bottom')

        plt.annotate("%.3f" % round(mins[mem], 3),
                xy=(mins[basem],mins[oursm]),
                xytext=(1, 0),
                textcoords='axes fraction', arrowprops=dict(facecolor='black', shrink=0.05), horizontalalignment='right', verticalalignment='bottom')

    mx = max(df[basem].max(), df[oursm].max()) + 20000
    mn = max(min(df[basem].min(), df[oursm].min()) - 8, 1)

    plt.xlim(mn, mx)
    plt.ylim(mn, mx)

    lims = [mn,mx]
    lims2 = [mn/2.0,mx/2.0]
    lims10 = [mn/10.0,mx/10.0]
    lims100 = [mn/100.0,mx/100.0]

    ax.plot(lims, lims, 'k-', alpha=1, zorder=0, label="$y = x$")
    ax.plot(lims, lims2, 'k-.', alpha=1, zorder=0, label="$y = 1/2 x$")
    ax.plot(lims, lims10, 'k--', alpha=1, zorder=0, label="$y = 1/10 x$")
    ax.plot(lims, lims100, 'k:', alpha=1, zorder=0, label="$y = 1/100 x$")

    plt.legend()

    print('Min, Max: ', df[mem].min(), df[mem].max())

    gm0 = scipy.stats.mstats.gmean(dfa[mem].values)
    gm = scipy.stats.mstats.gmean(df[mem].values)
    print('Geometric mean without NOT OKs in IKOS: ', gm0)
    print('Geometric mean: ', gm)

    r, c = df.shape
    df = df.sort_values(by=basem, ascending=False)[0:(df.shape[0])//4]
    gmq = scipy.stats.mstats.gmean(df[mem].values)
    print('Geometric mean (top 25%): ', gmq)
    if 't1' in what:
        plt.savefig('data/scat-memory-t1.png', bbox_inches='tight')
        plt.clf()
        print('\nFigure saved to "data/scat-memory-t1.png".')
    elif 't2' in what:
        plt.savefig('data/scat-memory-t2.png', bbox_inches='tight')
        plt.clf()
        print('\nFigure saved to "data/scat-memory-t2.png".')

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
