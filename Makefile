CC = mpicc -cc=clang
CFLAGS = -Wall -O2 -Xpreprocessor -fopenmp
LIBS = -lcrypto -lomp
OPENSSL_PATH = /opt/homebrew/opt/openssl@3

# Include paths for headers and library paths for linker
INCLUDES = -I$(OPENSSL_PATH)/include -I/opt/homebrew/Cellar/libomp/18.1.8/include
LDFLAGS = -L$(OPENSSL_PATH)/lib -L/opt/homebrew/Cellar/libomp/18.1.8/lib

# Names of the executables
EXECUTABLE_PARALLEL = my_program_parallel
EXECUTABLE_SEQUENTIAL = my_program_sequential

# Source files
SOURCE_PARALLEL = bruteforce_parallel.c
SOURCE_SEQUENTIAL = bruteforce_sequential.c

.PHONY: all clean run_parallel run_sequential

all: $(EXECUTABLE_PARALLEL) $(EXECUTABLE_SEQUENTIAL)

$(EXECUTABLE_PARALLEL):
	$(CC) -o $@ $(SOURCE_PARALLEL) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $(LIBS)

$(EXECUTABLE_SEQUENTIAL):
	$(CC) -o $@ $(SOURCE_SEQUENTIAL) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(EXECUTABLE_PARALLEL) $(EXECUTABLE_SEQUENTIAL)

run_parallel:
	mpirun -np 6 ./$(EXECUTABLE_PARALLEL) input1.txt output1.txt 123456L "dolor sit amet"

run_sequential:
	./$(EXECUTABLE_SEQUENTIAL) input.txt output.txt 123456L "dolor sit amet"
