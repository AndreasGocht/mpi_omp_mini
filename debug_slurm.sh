#!/bin/bash

#strace -f ./mpi_omp_mini 2>&1 | tee log-$SLURM_PROCID.log

port=10000 #$((SLURM_PROCID + 10000))
echo "# gdb -ex 'target remote  $(hostname):$port'"
export SHELL=/bin/bash
gdbserver :$port ./mpi_omp_mini
