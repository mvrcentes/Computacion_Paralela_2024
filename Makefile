CC = mpicc -cc=clang
CFLAGS = -Wall -O2 -Xpreprocessor -fopenmp
LIBS = -lcrypto -lomp
OPENSSL_PATH = /opt/homebrew/opt/openssl@3

# Include paths for headers and library paths for linker
INCLUDES = -I$(OPENSSL_PATH)/include -I/opt/homebrew/Cellar/libomp/18.1.8/include
LDFLAGS = -L$(OPENSSL_PATH)/lib -L/opt/homebrew/Cellar/libomp/18.1.8/lib

# Name of the executable
EXECUTABLE = my_program

# Source file
SOURCE = bruteforce00.c

all: $(EXECUTABLE)

$(EXECUTABLE):
	$(CC) -o $@ $(SOURCE) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(EXECUTABLE)

run:
	mpirun -np 6 ./$(EXECUTABLE) input1.txt output1.txt 123456L "dolor sit amet"