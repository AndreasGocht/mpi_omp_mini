#!/bin/bash

export OMPI_MCA_btl="^tcp"
#export OMPI_MCA_pml="ucx"
#export OMPI_MCA_osc="ucx"

echo "start"
./mpi_omp_mini 2>&1 | tee "pmix-debug-$(hostname).log"
echo "done"
