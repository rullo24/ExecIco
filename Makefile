# Compiler and Linker
CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lshlwapi

# Output
OUT = execico.exe

# Source files
SRC = main.c

# Object files
OBJ = $(SRC:.c=.o)

# Targets
all: $(OUT)

# Link the program
$(OUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUT) $(LDFLAGS)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	del $(OBJ) $(OUT)
	@if exist icon.rc del icon.rc
	@if exist icon.o del icon.o