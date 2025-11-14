#include "City.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

City::City(int width, int height) : width(width), height(height) {
    int id_counter = 0;
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            intersections.push_back({id_counter++, (double)i, (double)j});
        }
    }
}

City::City(int width, int height, int rank, int size) : width(width), height(height) {
    // For now, every process knows the whole map.
    // Partitioning will be done by assigning intersections to processes.
    int id_counter = 0;
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            intersections.push_back({id_counter++, (double)i, (double)j});
        }
    }

    adj.resize(intersections.size());
    int road_id_counter = 0;
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            int current_node = i * height + j;
            if (i + 1 < width) {
                int right_node = (i + 1) * height + j;
                adj[current_node].push_back({road_id_counter++, current_node, right_node, 1.0, 60.0, 10});
                adj[right_node].push_back({road_id_counter++, right_node, current_node, 1.0, 60.0, 10});
            }
            if (j + 1 < height) {
                int up_node = i * height + (j + 1);
                adj[current_node].push_back({road_id_counter++, current_node, up_node, 1.0, 60.0, 10});
                adj[up_node].push_back({road_id_counter++, up_node, current_node, 1.0, 60.0, 10});
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
    if (car.current_road_segment + 1 >= car.route.size()) return;

    int from = car.route[car.current_road_segment];
    int to = car.route[car.current_road_segment + 1];

    Road* current_road = nullptr;
    for(auto& road : adj[from]) {
        if(road.to_intersection == to) {
            current_road = &road;
            break;
        }
    }
    if(!current_road) return;

    double speed = current_road->speed_limit;
    double distance_to_travel = speed * time_step;

    car.position_on_road += distance_to_travel;
    if (car.position_on_road >= current_road->length) {
        car.current_road_segment++;
        car.position_on_road = 0;
    }
}

std::vector<int> City::generate_random_route(int start_node) {
    std::vector<int> route;
    route.push_back(start_node);
    int current_node = start_node;
    int route_length = (width + height) / 2;

    for (int i = 0; i < route_length; ++i) {
        if(adj[current_node].empty()) break;
        int next_node_idx = rand() % adj[current_node].size();
        current_node = adj[current_node][next_node_idx].to_intersection;

        // Avoid immediate revisits
        if (route.size() > 1 && current_node == route[route.size()-2]) {
             current_node = adj[current_node][(next_node_idx + 1) % adj[current_node].size()].to_intersection;
        }
        route.push_back(current_node);
    }
    return route;
}

void City::print_graph() {
    for (const auto& intersection : intersections) {
        std::cout << "Intersection " << intersection.id << " at (" << intersection.x << ", " << intersection.y << ")\n";
        for (const auto& road : adj[intersection.id]) {
            std::cout << "  -> Road to " << road.to_intersection << " (length: " << road.length << ", speed limit: " << road.speed_limit << ")\n";
        }
    }
}
