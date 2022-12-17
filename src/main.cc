#include <cstdio>
#include <cstdlib>
#include <string>
#include <mpi.h>

#include "raft.h"
#include "logger.h"

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "usage: ./raft <min_timeout> <max_timeout> <fault_chance> <log_file>");
        return -1;
    }

    /* Init parameters */
    int min_timeout = atoi(argv[1]);
    int max_timeout = atoi(argv[2]);
    float fail_chance = atof(argv[3]);
    std::string filename = argv[4];

    /* Init MPI */
    int mpi_comm_size, mpi_rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    //Initialize node
    filename.append(std::to_string(mpi_rank) + ".log");
    FILE *file = fopen(filename.c_str(), "w");
    Logger logger = Logger(file, info);
    Node raft_node(
        mpi_comm_size, 
        mpi_rank,
        min_timeout,
        max_timeout,
        FailType::chance, 
        fail_chance, 
        logger);

    //Synchronize node start times
    MPI_Barrier(MPI_COMM_WORLD);

    //Run the leader election algorithm
    raft_node.run();

    //Clean up
    fclose(file);
    MPI_Finalize();

    return 0;
}