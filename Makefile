# Variables
CC = mpicc
CFLAGS = -Wall -O2 -Xpreprocessor -fopenmp
LIBS = -lcrypto -lomp
OPENSSL_PATH = /opt/homebrew/opt/openssl@3

# Include paths for headers and library paths for linker
INCLUDES = -I$(OPENSSL_PATH)/include -I/opt/homebrew/opt/libomp/include
LDFLAGS = -L$(OPENSSL_PATH)/lib -L/opt/homebrew/opt/libomp/lib

.PHONY: all clean run_parallel run_sequential

# Define the source files and targets
SRC_1 = naive.c
TARGET_1 = naive

SRC_2 = naiveomp.c
TARGET_2 = naiveomp

SRC_3 = dbloques.c
TARGET_3 = dbloques

SRC_4 = secuencial.c
TARGET_4 = secuencial

# Default rule to compile all targets
all: $(TARGET_1) $(TARGET_2) $(TARGET_3) $(TARGET_4)

# Compile naive
$(TARGET_1): $(SRC_1)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET_1) $(SRC_1) $(LDFLAGS) $(LIBS)

# Compile naiveomp
$(TARGET_2): $(SRC_2)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET_2) $(SRC_2) $(LDFLAGS) $(LIBS)

# Compile dbloques
$(TARGET_3): $(SRC_3)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET_3) $(SRC_3) $(LDFLAGS) $(LIBS)

# Compile secuencial
$(TARGET_4): $(SRC_4)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET_4) $(SRC_4) $(LDFLAGS) $(LIBS)

# Run naive with 4 processes
run_alt1: $(TARGET_1)
	mpirun --allow-run-as-root -np 4 ./$(TARGET_1) medium.txt 123456789 "al mundo del"

# Run naiveomp with 4 processes
run_alt2: $(TARGET_2)
	mpirun --allow-run-as-root -np 4 ./$(TARGET_2) medium.txt 18014398519481984 "al mundo del"

# Run dbloques with 4 processes
run_div: $(TARGET_3)
	mpirun --allow-run-as-root -n 4 ./$(TARGET_3) medium.txt 36028798018963968L "al mundo del" 1000

# Run secuencial without MPI
run_seq: $(TARGET_4)
	./$(TARGET_4)

# Clean all compiled executables
clean:
	rm -f $(TARGET_1) $(TARGET_2) $(TARGET_3) $(TARGET_4)

# Phony targets to avoid conflicts with files
.PHONY: all run_alt1 run_alt2 run_div run_seq clean
