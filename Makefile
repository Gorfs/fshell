# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -Iinclude  # -Wall for warnings, -Werror to treat warnings as errors, -g for debugging, -I for include directory

# Source files (add more as needed)
SRCS = src/main.c 
OBJS = $(SRCS:.c=.o) # Object files

# Executable name
TARGET = fsh 

# Default target (builds the shell)
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@



# Clean target (removes object files and executable)
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets (always run, even if a file with the same name exists)
.PHONY: all clean
