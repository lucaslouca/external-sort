CC := clang++
CFLAGS := -Wall -g -std=c++20
TARGET := genapp

# Get all .cpp files from the current directory and dir "/xxx/xxx/"
SRCS := $(wildcard *.cpp)

# Substitute all ".cpp" file name strings to ".o" file name strings
OBJS := $(patsubst %.cpp, %.o, $(SRCS))

all: $(TARGET)

# Link: create an executable out of all the .o files
$(TARGET): $(OBJS)
	$(CC) -o $@ $^

# Compile every .cpp file into a .o file 
%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf $(TARGET) *.o

.PHONY: 
	all clean