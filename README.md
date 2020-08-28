# MIKOS Artifact for SAS 2020

This is the software artifact that accompanies the paper "Memory-Efficient Fixpoint Computation" by Sung Kook Kim, Arnaud J. Venet, and Aditya V. Thakur.

This consists of our memory efficient abstract interpreter MIKOS, benchmarks used in the experiments, and scripts to reproduce experiment results in the paper.

MIKOS is based on [IKOS](https://github.com/NASA-SW-VnV/ikos), an abstract interpretation
based static analysis tool.
Given a program as input, both IKOS and MIKOS compute invariants on each program point and run assertion checks on them.

## Docker

A docker image is provided with dependencies installed to run the artifact.

One can pull the image from the author's Docker Hub, or build it locally by the following commands:

```
$ docker pull skkeem/mikos
```
or
```
$ docker build -t skkeem/mikos .
```

See [docker installation](https://docs.docker.com/install/) on how to install `docker`.

## Benchmarks

To download and extract benchmarks in [`./benchmarks/`](benchmarks), run the following:

```
$ ./benchmarks/download_benchmarks.sh
```

## General MIKOS and IKOS Usage

To run MIKOS, first run the following command to run the docker image.

```
$ ./docker_run.sh
```

Once you are in the docker container, you become a `root` user as indicated by `#` in the following command lines.

Then, run the following to build `mikos`:
```
# ./setup.sh
```

Once the above is done, you can run MIKOS with a command `mikos`.

To test that MIKOS works, run the following to perform buffer overflow analysis
with interval domain on `test.c`.

```
# mikos -a=boa -d=interval ./benchmarks/test.c
```

If you see the following output, you are good to go.
MIKOS reports two occurrences of buffer overflow at line 8 and 9.

```
[*] Compiling ./benchmarks/test.c
[*] Running ikos preprocessor
[*] Running ikos analyzer
[*] Translating LLVM bitcode to AR
[*] Running liveness analysis
[*] Running widening hint analysis
[*] Running interprocedural value analysis (MIKOS)
[*] [MIKOS] Analyzing entry point 'main'

# Time stats:
clang        : 0.093 sec
ikos-analyzer: 0.015 sec
ikos-pp      : 0.012 sec

# Summary:
Total number of checks                : 4
Total number of unreachable checks    : 0
Total number of safe checks           : 2
Total number of definite unsafe checks: 2
Total number of warnings              : 0

The program is definitely UNSAFE

# Results
benchmarks/test.c: In function 'main':
benchmarks/test.c:8:10: error: buffer overflow, accessing index 10 of global variable 'a' of 10 elements
    a[i] = i;
         ^
benchmarks/test.c: In function 'main':
benchmarks/test.c:9:18: error: buffer overflow, accessing index 10 of global variable 'a' of 10 elements
    printf("%i", a[i]);
                 ^
```

You can also run the baseline IKOS with a `ikos` command.

```
# ikos -a=boa -d=interval ./benchmarks/test.c
```

```
[*] Compiling ./benchmarks/test.c
[*] Running ikos preprocessor
[*] Running ikos analyzer
[*] Translating LLVM bitcode to AR
[*] Running liveness analysis
[*] Running widening hint analysis
[*] Running interprocedural value analysis
[*] Analyzing entry point 'main'
[*] Checking properties for entry point 'main'

# Time stats:
clang        : 0.091 sec
ikos-analyzer: 0.010 sec
ikos-pp      : 0.012 sec

# Summary:
Total number of checks                : 4
Total number of unreachable checks    : 0
Total number of safe checks           : 2
Total number of definite unsafe checks: 2
Total number of warnings              : 0

The program is definitely UNSAFE

# Results
benchmarks/test.c: In function 'main':
benchmarks/test.c:8:10: error: buffer overflow, accessing index 10 of global variable 'a' of 10 elements
    a[i] = i;
         ^
benchmarks/test.c: In function 'main':
benchmarks/test.c:9:18: error: buffer overflow, accessing index 10 of global variable 'a' of 10 elements
    printf("%i", a[i]);
                 ^
```

## Running A Single Benchmark

### Task T1
Task T1 performs verification of user-provided assertions on SV-COMP 19 benchmarks (SVC benchmarks).
Reduced product of DBM with variable packing and congruence is used for task T1.
The following command performs the analysis on SVC benchmark, `linux-3.12-rc1.tar.xz-144_2a-drivers--staging--media--lirc--lirc_imon.ko-entry_point_false-unreach-call.cil.out.i`.

```
# mikos -a=prover -d=var-pack-dbm-congruence ./benchmarks/SVC/ldv-linux-3.12-rc1/linux-3.12-rc1.tar.xz-144_2a-drivers--staging--media--lirc--lirc_imon.ko-entry_point_false-unreach-call.cil.out.i
```

```
[*] Compiling ./benchmarks/SVC/ldv-linux-3.12-rc1/linux-3.12-rc1.tar.xz-144_2a-drivers--staging--media--lirc--lirc_imon.ko-entry_point_false-unreach-call.cil.out.i
clang: warning: argument unused during compilation: '-U _FORTIFY_SOURCE' [-Wunused-command-line-argument]
clang: warning: argument unused during compilation: '-isystem /mikos/build/run/include' [-Wunused-command-line-argument]
./benchmarks/SVC/ldv-linux-3.12-rc1/linux-3.12-rc1.tar.xz-144_2a-drivers--staging--media--lirc--lirc_imon.ko-entry_point_false-unreach-call.cil.out.i:3675:3: warning: unused label 'ERROR' [-Wunused-label]
  ERROR: ;
  ^~~~~~~
1 warning generated.
[*] Running ikos preprocessor
[*] Running ikos analyzer
[*] Translating LLVM bitcode to AR
[*] Running liveness analysis
[*] Running widening hint analysis
[*] Running interprocedural value analysis (MIKOS)
[*] [MIKOS] Analyzing entry point 'main'

# Time stats:
clang        : 0.127 sec
ikos-analyzer: 5.925 sec
ikos-pp      : 0.039 sec

# Summary:
Total number of checks                : 1
Total number of unreachable checks    : 0
Total number of safe checks           : 0
Total number of definite unsafe checks: 1
Total number of warnings              : 0

The program is definitely UNSAFE

# Results
benchmarks/SVC/ldv-linux-3.12-rc1/linux-3.12-rc1.tar.xz-144_2a-drivers--staging--media--lirc--lirc_imon.ko-entry_point_false-unreach-call.cil.out.i: In function 'ldv_error':
benchmarks/SVC/ldv-linux-3.12-rc1/linux-3.12-rc1.tar.xz-144_2a-drivers--staging--media--lirc--lirc_imon.ko-entry_point_false-unreach-call.cil.out.i:3676:3: error: assertion never holds
  __ikos_assert(0);
  ^
```

### Task T2
Task T2 proves the absence of buffer overflows on open-source benchmarks (OSS benchmarks).
Task T2 uses the interval domain was performed.
The following command performs the analysis on OSS benchmark, `hostapd_cli`.

```
# mikos -a=boa -d=interval ./benchmarks/OSS/hostapd-2.7/hostapd_cli.bc
```

```
[*] Running ikos preprocessor
[*] Running ikos analyzer
[*] Translating LLVM bitcode to AR
[*] Running liveness analysis
[*] Running widening hint analysis
[*] Running interprocedural value analysis (MIKOS)
[*] [MIKOS] Analyzing entry point 'main'

# Time stats:
ikos-analyzer: 6.103 sec
ikos-pp      : 0.062 sec

# Summary:
Total number of checks                : 2483
Total number of unreachable checks    : 0
Total number of safe checks           : 2065
Total number of definite unsafe checks: 0
Total number of warnings              : 418

The program is potentially UNSAFE

# Results
Report is too big (> 15 entries)

Use `ikos-report mikos-hostapd_cli.bc.db` to examine the report in your terminal.
Use `ikos-view mikos-hostapd_cli.bc.db` to examine the report in a web interface.
```

### Measuring memory reduction ratio (MRR) and speedup for a single benchmark

One can use the script `scripts/measure.py` to measure memory reduction ratio (MRR)
and speedup for a single benchmark.

The script takes two arguments. Task is determined by the benchmark used.

```
# ./scripts/measure.py <benchmark> <memory limit in GB>
```

For example, the following measures the MRR and speedup of performing task T1 on an SVC benchmark with 1 GB memory limit.
```
# ./scripts/measure.py ./benchmarks/SVC/ldv-linux-3.12-rc1/linux-3.12-rc1.tar.xz-144_2a-drivers--staging--media--lirc--lirc_imon.ko-entry_point_false-unreach-call.cil.out.i 1
```

It reports memory reduction of 0.08 and a speedup of 1.28.

```
Running IKOS...
Running MIKOS...
-------------------
IKOS peak memory usage (bytes):  589008896
MIKOS peak memory usage (bytes):  49385472
Memory reduction ratio (peak memory usage in MIKOS / peak memory usage in IKOS): 0.08
-------------------
IKOS analysis time (seconds):  6.995134016964585
MIKOS analysis time (seconds):  5.45805952232331
Speedup (analysis time in IKOS / analysis time in MIKOS): 1.28
```

## Reproducing Results

Reproducing the results are divided into steps: (1) reproducing raw data (CSV file)
and (2) generating figures and tables from the raw data.

The first step may take a lot of time if you choose to reproduce data for all benchmarks.
It would takes roughly **234 hours** to finish locally.
To reproduce results in a timely manner, authors recommend either reproducing data for a subset of benchmarks,
or using AWS instances as described in [`aws/README.md`](aws/README.md).

The second step shouldn't take much time and can be done locally.
One can choose to test the second part using the raw data authors provided.

It requires at least **64 GB** of memory to run the experiments.

### Step 0: Run docker image

First, run the following command to run the docker image containing MIKOS if you already haven't.

```
$ ./docker_run.sh
```

### Step 1: Reproduce raw data (CSV file)

This step runs both IKOS and MIKOS on the benchmarks and records their peak memory usage and analysis time.
The results will be saved in a csv file with following columns:

- `benchmark`: Name of the benchmark.
- `walltime (s)-ikos`: Analysis time in IKOS.
- `memory (MB)-ikos`: Peak memory usage in IKOS.
- `walltime (s)-mikos`: Analysis time in MIKOS.
- `memory (MB)-mikos`: Peak memory usage in MIKOS.

#### Reproduce data for a subset of benchmarks on a local machine (roughly takes 11 hours, 64 GB memory)

Because reproducing all data in the paper takes huge amount of time, we have provided
a subset of benchmarks that could be analyzed in a reasonable amount of time.
These are the benchmarks in Table 2 and 4.

Figures and tables generated using this data will be different from those in the paper.
If there is a `results` folder from a previous run, remove it before running the command.

```
# benchexec --no-container xml/t1-sub.xml; benchexec --no-container xml/t2-sub.xml
# ./scripts/csv.sh
```

#### Reproducing data for all benchmarks on a local machine (roughly takes 234 hours, 64 GB memory)

The following command reproduce all data.
If there is a `results` folder from a previous run, remove it before running the command.

```
# benchexec --no-container xml/t1.xml; benchexec --no-container xml/t2.xml
# ./scripts/csv.sh
```

#### Reproducing data using AWS instances

See [`aws/README.md`](aws/README.md).
Make sure to put the generated data in `data/`.

### Step 2: Reproduce figures & tables

After Step 1, two csv files, `t1.csv` and `t2.csv`, should be stored in `data/`.

If you skipped Step 1 to use the data provided by authors, run the following:
```
# mkdir data
# cp ./paper_data/*.csv data
```

The following generates scatter plots for memory footprint in Figure 3 and 4.
Figures are saved in `data/`.
```
# ./scripts/scat-memory.py
```
```
OK in IKOS also:  755
NOT OK in IKOS:  29
Min, Max:  0.001216800755 0.8952570177
Geometric mean without NOT OKs in IKOS:  0.04366281732611834
Geometric mean:  0.04071123324934014
Geometric mean (top 25%):  0.00923233813943994

Figure saved to "data/scat-memory-t1.png".
---------------------------
OK in IKOS also:  425
NOT OK in IKOS:  1
Min, Max:  0.02223678545 0.9976420001
Geometric mean without NOT OKs in IKOS:  0.4362178449409037
Geometric mean:  0.4370255523384128
Geometric mean (top 25%):  0.32574247007013574

Figure saved to "data/scat-memory-t2.png".
```

The following generates scatter plots for memory footprint in Figure 3 and 4.
Figures are saved in `data/`.
```
# ./scripts/scat-time.py
```
```
Min, Max:  0.8715434759999999 1.803006306
Geometric mean:  1.2929253924720334

Figure saved to "data/scat-time-t1.png".
---------------------------
Min, Max:  0.721220697 2.251237376
Geometric mean:  1.0572908732959991

Figure saved to "data/scat-time-t2.png".
```

The following generates entries for Table 2 and 4 in Appendix.
Entries are saved in `data/`.
Entries for Table 2 are in `t1-MRR.csv` and `t1-MF.csv`.
Entries for Table 4 are in `t2-MRR.csv`, `t2-MF.csv`,
Note that names of SVC benchmarks are abbreviated in the paper.
```
# ./scripts/sample-memory.py
```
```
Table entries saved in data/.
---------------------------
Table entries saved in data/.
```

The following generates entries for Table 3 and 5 in Appendix.
Entries are saved in `data/`.
Entries for Table 3 are in `t1-speedup-low.csv` and `t1-speedup-high.csv`.
Entries for Table 5 are in `t2-speedup-low.csv` and `t2-speedup-high.csv`.
Note that names of SVC benchmarks are abbreviated in the paper.
```
# ./scripts/sample-time.py
```
```
Table entries saved in data/.
---------------------------
Table entries saved in data/.
```

Finally, the following generates histogram in Figure 5 and Figure 6 (in Appendix).
Figures are saved in `data/`.
```
# ./scripts/hist.py
```
```
Quartile 1:  647.7987840000001
Quartile 2:  2688.01024
Quartile 3:  9789.405184000001
Number of benchmarks in quartile:  a   196
     memory (MB)-ikos
388         51.802112
214        642.121728
Number of benchmarks in quartile:  b   196
     memory (MB)-ikos
133        649.691136
124       2675.965952
Number of benchmarks in quartile:  c   196
     memory (MB)-ikos
638       2700.054528
623       9764.196352
Number of benchmarks in quartile:  d   196
     memory (MB)-ikos
54         9865.03168
746       64000.00000
Figures saved to "data/{t1, t2}-hist-[a-d].png".
--------------------------------
Quartile 1:  184.93439999999998
Quartile 2:  442.689536
Quartile 3:  1120.795648
Number of benchmarks in quartile:  a   107
     memory (MB)-ikos
331         44.077056
80         184.266752
Number of benchmarks in quartile:  b   106
     memory (MB)-ikos
81         186.937344
227        435.953664
Number of benchmarks in quartile:  c   106
     memory (MB)-ikos
116        449.425408
288       1084.948480
Number of benchmarks in quartile:  d   107
     memory (MB)-ikos
242       1132.744704
425      64000.000000
Figures saved to "data/{t1, t2}-hist-[a-d].png".
```

## People
- [Aditya V. Thakur](https://thakur.cs.ucdavis.edu/) can be reached at
  [avthakur@ucdavis.edu](mailto:avthakur@ucdavis.edu).
- Arnaud J. Venet can be reached at
  [ajv@fb.com](mailto:ajv@fb.com).
- [Sung Kook Kim](https://skkeem.github.io/) can be reached at
  [sklkim@ucdavis.edu](mailto:sklkim@ucdavis.edu).
