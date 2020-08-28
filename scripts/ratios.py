#!/usr/bin/env python3
import pandas as pd
import sys

if __name__ == '__main__':
    which = sys.argv[1]
    df = pd.read_csv(which)
    ikosm = 'memory (MB)-ikos'
    mikosm = 'memory (MB)-mikos'
    ikost = 'walltime (s)-ikos'
    mikost = 'walltime (s)-mikos'
    for i, r in df.iterrows():
        df.at[i, 'memreduction'] = r[mikosm] / r[ikosm]
        df.at[i, 'speedup'] = r[ikost] / r[mikost]
    df.to_csv(which, index=False)
