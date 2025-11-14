#include "City.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <queue>
#include <limits>

City::City(int width, int height) : width(width), height(height) {
    int id_counter = 0;
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            intersections.push_back({id_counter++, (double)i, (double)j});
        }
    }
    build_roads();
}

City::City(int width, int height, int rank, int size) 
    : width(width), height(height) {
    // Build complete city graph (all processes need to know the map)
    int id_counter = 0;
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            intersections.push_back({id_counter++, (double)i, (double)j});
        }
    }
    build_roads();
}

void City::build_roads() {
    adj.resize(intersections.size());
    int road_id_counter = 0;
    
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            int current_node = i * height + j;
            
            // Connect to right neighbor
            if (i + 1 < width) {
                int right_node = (i + 1) * height + j;
                adj[current_node].push_back({
                    road_id_counter++, 
                    current_node, 
                    right_node, 
                    1.0,  // length
                    0.5,  // speed (units per time_step)
                    10    // max_cars
                });
                adj[right_node].push_back({
                    road_id_counter++, 
                    right_node, 
                    current_node, 
                    1.0, 
                    0.5, 
                    10
                });
            }
            
            // Connect to upper neighbor
            if (j + 1 < height) {
                int up_node = i * height + (j + 1);
                adj[current_node].push_back({
                    road_id_counter++, 
                    current_node, 
                    up_node, 
                    1.0, 
                    0.5, 
                    10
                });
                adj[up_node].push_back({
                    road_id_counter++, 
                    up_node, 
                    current_node, 
                    1.0, 
                    0.5, 
                    10
                });
            }
        }
    }
}

int City::get_next_intersection(const Car& car) {
    if (car.current_road_segment + 1 < car.route.size()) {
        return car.route[car.current_road_segment + 1];
    }
    return -1; // End of route
}

void City::update_car_position(Car& car, double time_step) {
    // Check if car has finished its route
    if (car.current_road_segment + 1 >= car.route.size()) {
        return;
    }
    
    int from = car.route[car.current_road_segment];
    int to = car.route[car.current_road_segment + 1];
    
    // Find the road
    Road* current_road = nullptr;
    for (auto& road : adj[from]) {
        if (road.to_intersection == to) {
            current_road = &road;
            break;
        }
    }
    
    if (!current_road) {
        std::cerr << "Warning: No road from " << from << " to " << to << "\n";
        return;
    }
    
    // Move the car
    double speed = current_road->speed_limit;
    double distance_to_travel = speed * time_step;
    car.position_on_road += distance_to_travel;
    
    // Check if reached next intersection
    if (car.position_on_road >= current_road->length) {
        car.current_road_segment++;
        car.position_on_road = 0.0;
    }
}

std::vector<int> City::generate_random_route(int start_node) {
    std::vector<int> route;
    
    // Validate start node
    if (start_node < 0 || start_node >= intersections.size()) {
        start_node = 0;
    }
    
    route.push_back(start_node);
    int current_node = start_node;
    
    // Generate route with reasonable length
    int route_length = 15 + (rand() % 15);  // 15-30 steps
    
    for (int i = 0; i < route_length; ++i) {
        if (adj[current_node].empty()) break;
        
        // Pick a random neighbor
        int next_node_idx = rand() % adj[current_node].size();
        int next_node = adj[current_node][next_node_idx].to_intersection;
        
        // Avoid immediate backtracking
        if (route.size() > 1 && next_node == route[route.size() - 2]) {
            // Try another neighbor
            if (adj[current_node].size() > 1) {
                next_node_idx = (next_node_idx + 1) % adj[current_node].size();
                next_node = adj[current_node][next_node_idx].to_intersection;
            }
        }
        
        route.push_back(next_node);
        current_node = next_node;
    }
    
    return route;
}

void City::print_graph() {
    std::cout << "=== City Graph ===\n";
    std::cout << "Size: " << width << "x" << height 
              << " (" << intersections.size() << " intersections)\n\n";
    
    for (size_t i = 0; i < std::min(intersections.size(), size_t(5)); ++i) {
        const auto& intersection = intersections[i];
        std::cout << "Intersection " << intersection.id 
                  << " at (" << intersection.x << ", " << intersection.y << ")\n";
        for (const auto& road : adj[intersection.id]) {
            std::cout << "  -> " << road.to_intersection 
                      << " (len: " << road.length 
                      << ", speed: " << road.speed_limit << ")\n";
        }
    }
    std::cout << "... (" << intersections.size() - 5 << " more intersections)\n";
}
