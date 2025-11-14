# Compiler and flags
CXX = mpic++
CXXFLAGS = -std=c++11 -fopenmp -O3 -Wall -Wextra
TARGET = traffic_sim

# Directories
SRC_DIR = .
BUILD_DIR = ../build
BIN_DIR = ..

# Source files
SOURCES = main.cpp City.cpp
HEADERS = City.h
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(BIN_DIR)/$(TARGET)

# Build executable
$(BIN_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(BIN_DIR)/$(TARGET) $(OBJECTS)
	@echo "Build complete: $(BIN_DIR)/$(TARGET)"

# Compile object files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run with different configurations
run-serial:
	cd $(BIN_DIR) && mpirun -np 1 ./$(TARGET)

run-mpi:
	cd $(BIN_DIR) && mpirun -np 4 ./$(TARGET)

run-hybrid:
	cd $(BIN_DIR) && export OMP_NUM_THREADS=2 && mpirun -np 2 ./$(TARGET)

# Test different configurations
test: $(BIN_DIR)/$(TARGET)
	@echo "=== Testing Serial ==="
	@cd $(BIN_DIR) && export OMP_NUM_THREADS=1 && mpirun -np 1 ./$(TARGET)
	@echo "\n=== Testing 2 MPI Processes ==="
	@cd $(BIN_DIR) && export OMP_NUM_THREADS=1 && mpirun -np 2 ./$(TARGET)
	@echo "\n=== Testing Hybrid (2 procs x 2 threads) ==="
	@cd $(BIN_DIR) && export OMP_NUM_THREADS=2 && mpirun -np 2 ./$(TARGET)

# Quick test with few iterations (for debugging)
test-quick: $(BIN_DIR)/$(TARGET)
	@echo "Quick test (modify simulation_steps to 10 in main.cpp first)"
	cd $(BIN_DIR) && mpirun -np 2 ./$(TARGET)

# Visualization
visualize:
	cd $(BIN_DIR)/visualization && python3 visualize.py

# Clean build artifacts
clean:
	rm -f $(BIN_DIR)/$(TARGET) $(OBJECTS) $(BIN_DIR)/car_positions.csv
	rm -f *.o

# Deep clean (including visualization outputs)
distclean: clean
	rm -f $(BIN_DIR)/*.csv $(BIN_DIR)/*.dat
	rm -f $(BIN_DIR)/visualization/*.png $(BIN_DIR)/visualization/*.gif

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
