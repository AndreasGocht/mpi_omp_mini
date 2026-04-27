#!/bin/bash

strace -f ./mpi_omp_mini 2>&1 | tee log-$SLURM_TASK_PID.log
