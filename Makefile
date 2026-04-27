mpi_omp_mini: mpi_omp_mini.cpp
	mpic++ mpi_omp_mini.cpp -o mpi_omp_mini -fopenmp
clean:
	rm mpi_omp_mini
