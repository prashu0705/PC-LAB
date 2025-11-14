#ifndef CITY_H
#define CITY_H

#include <vector>
#include <iostream>

// Represents an intersection in the city
struct Intersection {
    int id;
    double x, y; // Coordinates in grid
};

// Represents a road connecting two intersections
struct Road {
    int id;
    int from_intersection;
    int to_intersection;
    double length;           // Distance between intersections
    double speed_limit;      // Maximum speed on this road
    int max_cars;           // Capacity (for congestion modeling)
};

// Represents a car in the simulation
struct Car {
    int id;
    std::vector<int> route;        // Sequence of intersection IDs
    int current_road_segment;      // Index in route (which road we're on)
    double position_on_road;       // Position along current road segment
};

class City {
public:
    // Constructors
    City(int width, int height);                          // Serial version
    City(int width, int height, int rank, int size);     // MPI version
    
    // Core functionality
    int get_next_intersection(const Car& car);
    void update_car_position(Car& car, double time_step);
    std::vector<int> generate_random_route(int start_node);
    void print_graph();

private:
    std::vector<Intersection> intersections;
    std::vector<std::vector<Road>> adj;  // Adjacency list
    int width, height;
    
    void build_roads();  // Helper to construct road network
};

#endif // CITY_H
