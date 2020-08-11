#!/bin/bash

docker run --rm -v /sys/fs/cgroup:/sys/fs/cgroup:rw -v `pwd`:/mikos:rw -w /mikos -it --privileged skkeem/mikos
