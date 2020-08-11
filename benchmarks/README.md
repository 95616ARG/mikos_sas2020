# Benchmarks

Benchmarks come from the following two sources:

1. [Arch Linux core packages](https://wiki.archlinux.org/index.php/Official_repositories#core) (OSS)
2. [SV-COMP 2019](https://github.com/sosy-lab/sv-benchmarks) (SVC)

Benchmark categories used in SV-COMP 19 are listed in `svc.category`.

## OSS benchmark selection
- `oss-full.set` : Full list of benchmarks in the above categories.
This includes benchmarks that had errors in the baseline IKOS.
- `oss-err.set` : List of benchmarks that had errors in the baseline IKOS.
- `oss-initial.set` : `oss-full.set` - `oss.err.set`
- `oss-to.set` : List of benchmarks that timed out in MIKOS. These benchmarks failed
to complete in IKOS (time out or space out).
- `oss-so.set` : List of benchmarks that spaced out in MIKOS. These benchmarks failed
to complete in IKOS (time out or space out).
- `oss-5s.set` : List of benchmarks that took less than 5 secons in IKOS.
- `oss.set` : `oss-initial.set` - `oss-to.set` - `oss-so.set` - `oss-5s.set`

## SVC benchmark selection
- `svc.category` : Benchmark categories used.
- `svc-full.set` : Full list of benchmarks in the above categories.
This includes benchmarks that had errors in the baseline IKOS.
- `svc-err.set` : List of benchmarks that had errors in the baseline IKOS.
- `svc-initial.set` : `svc-full.set` - `svc.err.set`
- `svc-to.set` : List of benchmarks that timed out in MIKOS. These benchmarks failed
to complete in IKOS (time out or space out).
- `svc-so.set` : List of benchmarks that spaced out in MIKOS. These benchmarks failed
to complete in IKOS (time out or space out).
- `svc-5s.set` : List of benchmarks that took less than 5 secons in IKOS.
- `svc.set` : `svc-initial.set` - `svc-to.set` - `svc-so.set` - `svc-5s.set`
