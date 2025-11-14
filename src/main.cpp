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

    // Partition the city among processes
    City city(10, 10, rank, size);

    int intersections_per_process = 100 / size;
    int start_node = rank * intersections_per_process;
    int end_node = (rank + 1) * intersections_per_process;

    // Create cars for the local process
    std::vector<Car> local_cars;
    int cars_per_process = 100 / size;
    for (int i = 0; i < cars_per_process; ++i) {
        int start_intersection = start_node + (rand() % intersections_per_process);
        local_cars.push_back({rank * cars_per_process + i, 
                              city.generate_random_route(start_intersection), 
                              0, 0.0});
    }

    // Simulation parameters
    int simulation_steps = 1000;
    double time_step = 0.1;

    double start_time = MPI_Wtime();

    std::ofstream outfile;
    if (rank == 0) {
        outfile.open("car_positions.csv");
        outfile << "step,car_id,position,intersection\n";
    }

    // Main simulation loop
    for (int step = 0; step < simulation_steps; ++step) {
        
        // FIX #1: Parallel car updates (thread-safe)
        #pragma omp parallel for schedule(dynamic)
        for (size_t j = 0; j < local_cars.size(); ++j) {
            city.update_car_position(local_cars[j], time_step);
        }

        // FIX #2: Serial boundary detection (after parallel updates)
        std::vector<Car> to_send;
        for (auto it = local_cars.begin(); it != local_cars.end(); ) {
            if (it->current_road_segment >= it->route.size()) {
                // Car finished route
                it = local_cars.erase(it);
                continue;
            }
            
            int current_intersection = it->route[it->current_road_segment];
            if (current_intersection < start_node || current_intersection >= end_node) {
                to_send.push_back(*it);
                it = local_cars.erase(it);
            } else {
                ++it;
            }
        }

        // Send cars that left this region
        for (const auto& car_to_send : to_send) {
            int current_intersection = car_to_send.route[car_to_send.current_road_segment];
            int dest_rank = current_intersection / intersections_per_process;
            
            // Clamp to valid rank range
            if (dest_rank < 0) dest_rank = 0;
            if (dest_rank >= size) dest_rank = size - 1;
            
            // Send car data with unique tags
            MPI_Send(&car_to_send.id, 1, MPI_INT, dest_rank, 0, MPI_COMM_WORLD);
            MPI_Send(&car_to_send.current_road_segment, 1, MPI_INT, dest_rank, 1, MPI_COMM_WORLD);
            MPI_Send(&car_to_send.position_on_road, 1, MPI_DOUBLE, dest_rank, 2, MPI_COMM_WORLD);
            int route_size = car_to_send.route.size();
            MPI_Send(&route_size, 1, MPI_INT, dest_rank, 3, MPI_COMM_WORLD);
            MPI_Send(car_to_send.route.data(), route_size, MPI_INT, dest_rank, 4, MPI_COMM_WORLD);
        }

        // FIX #3: Receive ALL incoming cars (not just one)
        while (true) {
            MPI_Status status;
            int flag = 0;
            MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &flag, &status);
            
            if (!flag) break;  // No more incoming cars
            
            Car received_car;
            int source_rank = status.MPI_SOURCE;
            
            MPI_Recv(&received_car.id, 1, MPI_INT, source_rank, 0, 
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&received_car.current_road_segment, 1, MPI_INT, source_rank, 1, 
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&received_car.position_on_road, 1, MPI_DOUBLE, source_rank, 2, 
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            int route_size;
            MPI_Recv(&route_size, 1, MPI_INT, source_rank, 3, 
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            received_car.route.resize(route_size);
            MPI_Recv(received_car.route.data(), route_size, MPI_INT, source_rank, 4, 
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            local_cars.push_back(received_car);
        }

        // FIX #4: Synchronize before gathering data
        MPI_Barrier(MPI_COMM_WORLD);

        // FIX #5: Complete data gathering on rank 0
        if (rank == 0) {
            // Write rank 0's cars
            for (const auto& car : local_cars) {
                if (car.current_road_segment < car.route.size()) {
                    outfile << step << "," 
                            << car.id << "," 
                            << car.position_on_road << "," 
                            << car.route[car.current_road_segment] << "\n";
                }
            }

            // Receive and write other ranks' cars
            for (int source_rank = 1; source_rank < size; ++source_rank) {
                int cars_from_source;
                MPI_Recv(&cars_from_source, 1, MPI_INT, source_rank, 5, 
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                for (int j = 0; j < cars_from_source; ++j) {
                    int car_id, current_intersection;
                    double position;
                    
                    MPI_Recv(&car_id, 1, MPI_INT, source_rank, 6, 
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(&position, 1, MPI_DOUBLE, source_rank, 7, 
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(&current_intersection, 1, MPI_INT, source_rank, 8, 
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    
                    outfile << step << "," 
                            << car_id << "," 
                            << position << "," 
                            << current_intersection << "\n";
                }
            }
        } else {
            // Send number of cars
            int num_local_cars = local_cars.size();
            MPI_Send(&num_local_cars, 1, MPI_INT, 0, 5, MPI_COMM_WORLD);
            
            // Send each car's data
            for (const auto& car : local_cars) {
                if (car.current_road_segment < car.route.size()) {
                    int current_intersection = car.route[car.current_road_segment];
                    MPI_Send(&car.id, 1, MPI_INT, 0, 6, MPI_COMM_WORLD);
                    MPI_Send(&car.position_on_road, 1, MPI_DOUBLE, 0, 7, MPI_COMM_WORLD);
                    MPI_Send(&current_intersection, 1, MPI_INT, 0, 8, MPI_COMM_WORLD);
                }
            }
        }

        // Progress indicator
        if (rank == 0 && step % 100 == 0) {
            std::cout << "Step " << step << "/" << simulation_steps 
                      << " (Local cars: " << local_cars.size() << ")\n";
        }
    }

    // Finalization
    if (rank == 0) {
        outfile.close();
        double end_time = MPI_Wtime();
        std::cout << "\n=== Simulation Complete ===\n";
        std::cout << "Total time: " << end_time - start_time << " seconds\n";
        std::cout << "Output saved to: car_positions.csv\n";
    }

    MPI_Finalize();
    return 0;
}
