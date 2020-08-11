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

def plot1(dff, basem, mem, oss, idx):
    print("Number of benchmarks in quartile: ", idx, " ", dff.shape[0])
    print(dff.sort_values(by=basem).iloc[[0, -1]][[basem]])

    ax = plt.subplot(111)
    n, bins, patches = ax.hist(dff[mem].values, bins=100, range=[0, 1], edgecolor='black', alpha=0.8)
    if oss:
        plt.yticks([0, 5, 10, 15, 20])
    else:
        plt.yticks([0, 20, 40, 60, 80, 100, 120])
    plt.grid(b=True, zorder=0, ls='--', alpha=0.3)
    plt.xlabel(r'Memory usage of \textsc{Mikos} / Memory usage of \textsc{ikos}')
    plt.ylabel(r'Frequency')
    gm = scipy.stats.mstats.gmean(dff[mem].values)

    xcor = 0.45
    plt.text(xcor, 0.95, "Max. reduction = %.3f\nMin. reduction = %.3f\n Avg. reduction = %.3f" % (round(dff[mem].min(), 3), round(dff[mem].max(), 3), round(gm, 3)),
    transform=ax.transAxes, verticalalignment='top')
    if oss:
        plt.savefig('data/hist-t2-%s.png' % idx, bbox_inches='tight')
    else:
        plt.savefig('data/hist-t1-%s.png' % idx, bbox_inches='tight')
    plt.clf()

def plot(what):
    df = pd.read_csv(what)
    baset = 'walltime (s)-ikos'
    ourst = 'walltime (s)-mikos'
    basem = 'memory (MB)-ikos'
    oursm = 'memory (MB)-mikos'
    mem = 'memreduction'

    df = df[~df['status-ikos'].str.startswith('ERROR')]
    df = df[~df['status-mikos'].str.startswith('ERROR')]
    df = df[df['status-mikos'].str.startswith('OK')]
    df = df[df[baset] >= 5]

    q25, q50, q75 = df[basem].quantile([0.25, 0.5, 0.75])
    print("Quartile 1: ", q25)
    print("Quartile 2: ", q50)
    print("Quartile 3: ", q75)

    df0 = df[df[basem] < q25]
    df1 = df[(q25 <= df[basem]) & (df[basem] < q50)]
    df2 = df[(q50 <= df[basem]) & (df[basem] < q75)]
    df3 = df[q75 <= df[basem]]

    plot1(df0, basem, mem, 't2' in what, 'a')
    plot1(df1, basem, mem, 't2' in what, 'b')
    plot1(df2, basem, mem, 't2' in what, 'c')
    plot1(df3, basem, mem, 't2' in what, 'd')

    print('Figures saved to "data/{t1, t2}-hist-[a-d].png".')

if __name__ == '__main__':
    mpl.rcParams['text.usetex'] = True
    mpl.rcParams['text.latex.preamble'] = [r'\usepackage{amsmath}']
    mpl.rcParams['font.size'] = 20.0
    mpl.rcParams['axes.labelsize'] = 20.0
    mpl.rcParams['ytick.labelsize'] = 20.0
    mpl.rcParams['xtick.labelsize'] = 20.0
    mpl.rcParams['font.weight'] = 'bold'

    plot('data/t1.csv')
    print("--------------------------------")
    plot('data/t2.csv')
