mpi_omp_mini:
	mpicc mpi_omp_mini.c -o mpi_omp_mini -fopenmp 
clean:
	rm mpi_omp_mini
