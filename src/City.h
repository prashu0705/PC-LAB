#ifndef CITY_H
#define CITY_H

#include <vector>
#include <iostream>

// Represents an intersection in the city
struct Intersection {
    int id;
    double x, y; // Coordinates
};

// Represents a road connecting two intersections
struct Road {
    int id;
    int from_intersection;
    int to_intersection;
    double length;
    double speed_limit;
    int max_cars;
};

// Represents a car in the simulation
struct Car {
    int id;
    std::vector<int> route; // List of intersection IDs
    int current_road_segment; // Index in the route
    double position_on_road;
};

class City {
public:
    // Constructor for serial version
    City(int width, int height);

    // Constructor for MPI version
    City(int width, int height, int rank, int size);

    // Get the next intersection in a car's route
    int get_next_intersection(const Car& car);

    // Update a car's position
    void update_car_position(Car& car, double time_step);

    // Generate a random route for a car
    std::vector<int> generate_random_route(int start_node);

    void print_graph();

private:
    std::vector<Intersection> intersections;
    std::vector<std::vector<Road>> adj;
    int width, height;
};

#endif // CITY_H
