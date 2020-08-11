#!/bin/bash

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")

cd $SCRIPT_DIR/..
./scripts/scat-memory.py
./scripts/scat-time.py
./scripts/sample-memory.py
./scripts/sample-time.py
./scripts/hist.py
