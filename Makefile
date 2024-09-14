CC = mpicc
CFLAGS = -Wall -O2 -Wno-deprecated-declarations
LIBS = -lcrypto
OPENSSL_PATH = /opt/homebrew/opt/openssl@3

# Include paths for headers and library paths for linker
INCLUDES = -I$(OPENSSL_PATH)/include
LDFLAGS = -L$(OPENSSL_PATH)/lib

# Name of the executable
EXECUTABLE = my_program

# Source file
SOURCE = bruteforce00.c

# Default target
all: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCE)
	$(CC) -o $@ $^ $(CFLAGS) $(INCLUDES) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(EXECUTABLE)

# To execute, you can add a run rule if necessary
run:
	mpirun -np 6 ./$(EXECUTABLE) input1.txt output1.txt 123456L "dolor sit amet"
