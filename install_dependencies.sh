#!/bin/bash
cd /tmp

# Upgrade
apt-get update
apt-get upgrade -y

apt-get install -y software-properties-common

echo "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-9 main" >> /etc/apt/sources.list
apt-get install -y wget gnupg
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
apt-get update

# Install all dependencies
apt-get install -y cmake libgmp-dev \
   libboost-dev libboost-filesystem-dev libboost-test-dev \
   python3 python3-dev python3-pip \
   git curl autoconf automake m4 libtool pkg-config \
   libsqlite3-dev libz-dev libedit-dev llvm-9 llvm-9-dev llvm-9-tools clang-9 \
   texlive dvipng texlive-latex-extra cm-super

# BenchExec
wget https://github.com/sosy-lab/benchexec/releases/download/2.5/benchexec_2.5-1_all.deb
apt-get install -y --install-recommends ./benchexec_2.5-1_all.deb
adduser root benchexec

# Python deps
python3 -m pip install pandas matplotlib scipy --user
