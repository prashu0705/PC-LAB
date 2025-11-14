#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <mpi.h>
#include <omp.h>
#include <fstream>
#include "City.h"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    srand(time(0) + rank);

    // Partition the city among processes.
    City city(10, 10, rank, size);

    int intersections_per_process = 100 / size;
    int start_node = rank * intersections_per_process;
    int end_node = (rank + 1) * intersections_per_process;

    // Create cars, but only for the local process
    std::vector<Car> local_cars;
    int cars_per_process = 100 / size;
    for (int i = 0; i < cars_per_process; ++i) {
        int start_node = rand() % 100;
        local_cars.push_back({rank * cars_per_process + i, city.generate_random_route(start_node), 0, 0.0});
    }

    // Simulation loop
    int simulation_steps = 1000;
    double time_step = 0.1;

    double start_time, end_time;
    if (rank == 0) {
        start_time = MPI_Wtime();
    }

    std::ofstream outfile;
    if (rank == 0) {
        outfile.open("car_positions.csv");
    }

    for (int i = 0; i < simulation_steps; ++i) {
    #pragma omp parallel for
        for (int j = 0; j < local_cars.size(); ++j) {
            city.update_car_position(local_cars[j], time_step);
        }

        std::vector<Car> to_send;
        for (auto it = local_cars.begin(); it != local_cars.end(); ) {
            int current_intersection = it->route[it->current_road_segment];
            if (current_intersection < start_node || current_intersection >= end_node) {
                to_send.push_back(*it);
                it = local_cars.erase(it);
            } else {
                ++it;
            }
        }

        for (const auto& car_to_send : to_send) {
            int dest_rank = car_to_send.route[car_to_send.current_road_segment] / intersections_per_process;
            // Serialize and send the car data
            MPI_Send(&car_to_send.id, 1, MPI_INT, dest_rank, 0, MPI_COMM_WORLD);
            MPI_Send(&car_to_send.current_road_segment, 1, MPI_INT, dest_rank, 1, MPI_COMM_WORLD);
            MPI_Send(&car_to_send.position_on_road, 1, MPI_DOUBLE, dest_rank, 2, MPI_COMM_WORLD);
            int route_size = car_to_send.route.size();
            MPI_Send(&route_size, 1, MPI_INT, dest_rank, 3, MPI_COMM_WORLD);
            MPI_Send(car_to_send.route.data(), route_size, MPI_INT, dest_rank, 4, MPI_COMM_WORLD);
        }

        // Check for and receive incoming cars
        MPI_Status status;
        int flag = 0;
        MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &flag, &status);
        if (flag) {
            Car received_car;
            int source_rank = status.MPI_SOURCE;
            MPI_Recv(&received_car.id, 1, MPI_INT, source_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&received_car.current_road_segment, 1, MPI_INT, source_rank, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&received_car.position_on_road, 1, MPI_DOUBLE, source_rank, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            int route_size;
            MPI_Recv(&route_size, 1, MPI_INT, source_rank, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            received_car.route.resize(route_size);
            MPI_Recv(received_car.route.data(), route_size, MPI_INT, source_rank, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            local_cars.push_back(received_car);
        }

        // Gather all car data on rank 0
        if (rank == 0) {
            int total_cars = local_cars.size();
            for (int source_rank = 1; source_rank < size; ++source_rank) {
                int cars_from_source;
                MPI_Recv(&cars_from_source, 1, MPI_INT, source_rank, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for(int j=0; j<cars_from_source; ++j) {
                    // This is simplified. In a real scenario, you'd receive car data.
                }
            }

            for (const auto& car : local_cars) {
                outfile << i << "," << car.id << "," << car.position_on_road << "," << car.route[car.current_road_segment] << std::endl;
            }
        } else {
            int num_local_cars = local_cars.size();
            MPI_Send(&num_local_cars, 1, MPI_INT, 0, 5, MPI_COMM_WORLD);
        }
    }

    if (rank == 0) {
        outfile.close();
        end_time = MPI_Wtime();
        std::cout << "Hybrid simulation finished in " << end_time - start_time << " seconds.\n";
    }

    MPI_Finalize();
    return 0;
}
