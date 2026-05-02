#ifndef _GNU_SOURCE
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <cstdlib>
#include <sstream>
#include <string>
#endif

#include <mpi.h>
#include <limits.h>
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>
#include <omp.h>
#include <stdlib.h>
#include <omp.h>
#include <sstream>
#include <list>
#include <iostream>
#include <chrono>
#include <thread>
#include "nlohmann/json.hpp"
using json = nlohmann::json;
using namespace std::chrono_literals;


#if not __GLIBC_PREREQ(2,30)
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

void send_message(int to_rank, std::string message)
{
    MPI_Send(message.c_str(), message.size(),  MPI_CHARACTER, to_rank, 0, MPI_COMM_WORLD);
}

std::string wait_for_message(int from_rank)
{
    int ping = 0;
    int msg_len = 0;
    MPI_Status status;
    MPI_Probe(from_rank, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status,  MPI_CHARACTER, &msg_len);
    char* msg = (char*)calloc(msg_len, sizeof(char));
    MPI_Recv(msg, msg_len, MPI_CHARACTER, from_rank, 0, MPI_COMM_WORLD, &status);
    std::string ret(msg, msg_len);
    free(msg);
    return ret;
}


int main(int argc, char *argv[]) {
    int rank, size;

    // Initialize MPI environment
    MPI_Init(&argc, &argv);

    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Get the rank of this process
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    std::string in;
    json j;

    if (size > 1)
    {
        if (rank != 0 )
        {
            in = wait_for_message(rank - 1);
            j = j.parse(in);
            j.push_back(json::object());
            assert(j.size() == rank+1);
            j[rank]["rank"] = rank;
            j[rank]["size"] = size;
        }
        else {
            j.push_back(json::object());
            assert(j.size() == rank+1);
            j[rank]["rank"] = rank;
            j[rank]["size"] = size;
        }
    }
    else {
        j.push_back(json::object());
        assert(j.size() == rank+1);
        j[rank]["rank"] = 0;
        j[rank]["size"] = 0;
    }


    char hostname[HOST_NAME_MAX+1];
    gethostname(hostname, HOST_NAME_MAX+1);
    std::string hostname_s = hostname;
    j[rank]["hostname"] = hostname_s;

    int omp_num_threads = omp_get_num_threads();

#pragma omp parallel
{
    omp_num_threads = omp_get_num_threads();
}
    j[rank]["num_threads"] = omp_num_threads;
    int* ready =(int*)calloc(omp_num_threads, sizeof(int));
#pragma omp parallel shared(ready, j)
{
    int omp_thread_num = omp_get_thread_num();
    int omp_num_threads = omp_get_num_threads();

{
    if (omp_thread_num != 0)
    {
        while (!ready[omp_thread_num-1]) {
            std::this_thread::sleep_for(10ms);
        }
    }
    j[rank]["threads"].push_back(json::object());
    assert(j[rank]["threads"].size() == omp_thread_num+1);
    j[rank]["threads"][omp_thread_num]["thread_num"] = omp_thread_num;

    cpu_set_t set;
    CPU_ZERO(&set);

    sched_getaffinity(gettid(), sizeof(set), &set);

    for(int i = 0; i<64; i++)
    {
        if (CPU_ISSET(i, &set))
        {
            j[rank]["threads"][omp_thread_num]["affinity"].push_back(i);
        }
    }

    ready[omp_thread_num] = 1;
}
}

    std::string output = j.dump();

    if (size > 1)
    {
        if (rank == 0)
        {
            send_message(rank + 1, output);
            std::string in = wait_for_message(size - 1);
            j = j.parse(in);
        } else {
            if (rank != size - 1)
            {
                send_message(rank + 1, output);
            }
            else
            {
                send_message(0, output);
            }
        }
    }

    if (rank == 0)
    {
        std::cout << j.dump(4);
    }
    // Finalize MPI environment
    MPI_Finalize();
    return 0;
}
