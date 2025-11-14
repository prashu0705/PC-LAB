#include <mpi.h>
#include <cassert>
#include "../src/City.h"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        if (rank == 0) {
            std::cerr << "This test requires exactly 2 processes.\\n";
        }
        MPI_Finalize();
        return 1;
    }

    City city(10, 10, rank, size);
    Car car;
    if (rank == 0) {
        // Create a car that will cross the boundary
        car = {0, {49, 50}, 0, 0.0};

        // Manually send the car
        int dest_rank = 1;
        MPI_Send(&car.id, 1, MPI_INT, dest_rank, 0, MPI_COMM_WORLD);
    } else {
        // Receive the car
        MPI_Recv(&car.id, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        assert(car.id == 0);
    }

    if(rank == 0) {
        std::cout << "Test passed: MPI communication seems to be working.\\n";
    }

    MPI_Finalize();
    return 0;
}
