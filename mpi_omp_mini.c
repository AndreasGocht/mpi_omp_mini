#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <mpi.h>
#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>
#include <omp.h>
#include <stdlib.h>

#if __GLIBC_PREREQ(2,30)
#define _GNU_SOURCE
#include <unistd.h>

#else

#include <sys/syscall.h>

pid_t
gettid(void)
{

    return syscall(SYS_gettid);
}
#endif

void send_ping(int to_rank)
{
    int ping = 0;
    MPI_Send(&ping, 1, MPI_INT, to_rank, 0, MPI_COMM_WORLD);
}

void wait_for_ping(int from_rank)
{
    int ping = 0;
    MPI_Status status;
    MPI_Recv(&ping, 1, MPI_INT, from_rank, 0, MPI_COMM_WORLD, &status);
}


int main(int argc, char *argv[]) {
    int rank, size;
    char hostname[1024];

    hostname[1023] = '\0';
    gethostname(hostname, 1023);

    // Initialize MPI environment
    printf("Start MPI_Init on host: %s\n", hostname);
    MPI_Init(&argc, &argv);
    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Get the rank of this process
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
    {
        printf("Successfully Initialized\n");
        printf("World size: %d\n", size);
    }


    printf("Rank: %d\n", rank);

    if (rank != 0 && size > 1)
    {
        wait_for_ping(rank - 1);
    }
    printf("On Rank %d out of %d:\n", rank, size);

    int omp_num_threads = omp_get_num_threads();
#pragma omp parallel
{
    omp_num_threads = omp_get_num_threads();
}
    int* ready =(int*)calloc(omp_num_threads, sizeof(int));
#pragma omp parallel shared(ready)
{
    int omp_thread_num = omp_get_thread_num();
    int omp_num_threads = omp_get_num_threads();

{
    if (omp_thread_num != 0)
    {
        while (!ready[omp_thread_num-1]) {}
    }

    printf("\tOn Thread %d out of %d\n",omp_thread_num, omp_num_threads);
    printf("\t\t\tAffinity: ");

    cpu_set_t set;
    CPU_ZERO(&set);

    sched_getaffinity(gettid(), sizeof(set), &set);

    for(int i = 0; i<64; i++)
    {
        if (CPU_ISSET(i, &set))
        {
            printf("%d,",i);
        }
    }
    printf("\n");
    ready[omp_thread_num] = 1;
}
}

    if (size > 1)
    {
        if (rank == 0)
        {
            send_ping(rank + 1);
            wait_for_ping(size - 1);
        } else {
            if (rank != size - 1)
            {
                send_ping(rank + 1);
            }
            else
            {
                send_ping(0);
            }

        }
    }


    // Finalize MPI environment
    MPI_Finalize();
    return 0;
}
