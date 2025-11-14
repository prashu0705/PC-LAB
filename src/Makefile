# Compiler and flags
CXX = mpic++
CXXFLAGS = -std=c++11 -fopenmp -O3 -Wall -Wextra
TARGET = traffic_sim

# Source files
SOURCES = main.cpp City.cpp
HEADERS = City.h
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)
	@echo "Build complete: ./$(TARGET)"

# Compile object files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run with different configurations
run-serial:
	mpirun -np 1 ./$(TARGET)

run-mpi:
	mpirun -np 4 ./$(TARGET)

run-hybrid:
	export OMP_NUM_THREADS=2 && mpirun -np 2 ./$(TARGET)

# Test different configurations
test: $(TARGET)
	@echo "=== Testing Serial ==="
	@export OMP_NUM_THREADS=1 && mpirun -np 1 ./$(TARGET)
	@echo "\n=== Testing 2 MPI Processes ==="
	@export OMP_NUM_THREADS=1 && mpirun -np 2 ./$(TARGET)
	@echo "\n=== Testing Hybrid (2 procs x 2 threads) ==="
	@export OMP_NUM_THREADS=2 && mpirun -np 2 ./$(TARGET)

# Quick test with few iterations (for debugging)
test-quick: $(TARGET)
	@echo "Quick test (modify simulation_steps to 10 in main.cpp first)"
	mpirun -np 2 ./$(TARGET)

# Visualization
visualize:
	cd visualization && python3 visualize.py

# Clean build artifacts
clean:
	rm -f $(TARGET) $(OBJECTS) car_positions.csv
	rm -f visualization/*.png visualization/*.gif

# Deep clean (including visualization outputs)
distclean: clean
	rm -f *.csv *.dat
	rm -f visualization/*.png visualization/*.gif

# Check if required tools are installed
check-env:
	@echo "Checking environment..."
	@which mpic++ > /dev/null || (echo "ERROR: mpic++ not found. Install OpenMPI." && exit 1)
	@echo "mpic++ found: $$(which mpic++)"
	@mpic++ --version | head -n 1
	@echo "\nChecking OpenMP support..."
	@echo "int main() { return 0; }" | mpic++ -fopenmp -x c++ - -o /tmp/omp_test 2>&1 && echo "OpenMP: OK" || echo "OpenMP: FAILED"
	@rm -f /tmp/omp_test
	@echo "\nEnvironment check complete!"

# Help
help:
	@echo "Available targets:"
	@echo "  make              - Build the simulation"
	@echo "  make run-serial   - Run with 1 process"
	@echo "  make run-mpi      - Run with 4 processes"
	@echo "  make run-hybrid   - Run with 2 processes, 2 threads each"
	@echo "  make test         - Run all configurations"
	@echo "  make visualize    - Generate plots from results"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make check-env    - Verify MPI and OpenMP are installed"
	@echo "  make help         - Show this help message"

.PHONY: all run-serial run-mpi run-hybrid test test-quick visualize clean distclean check-env help
