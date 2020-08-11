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

if __name__ == '__main__':
    which = sys.argv[1]
    df = pd.read_csv(which + '.csv')
    ikosm = 'memory (MB)-ikos'
    mikosm = 'memory (MB)-mikos'
    ikost = 'walltime (s)-ikos'
    mikost = 'walltime (s)-mikos'
    for i, r in df.iterrows():
        df.at[i, 'memreduction'] = r[mikosm] / r[ikosm]
        df.at[i, 'speedup'] = r[ikost] / r[mikost]
    df.to_csv(which + '.csv', index=False)
