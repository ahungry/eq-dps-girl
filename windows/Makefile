# Makefile for Project "dpsgirl"

# Compiler and linker settings
# CC = g++
CC = x86_64-w64-mingw32-gcc
CFLAGS = -Wall -Wextra -O2 # Release configuration flags
CFLAGS_DEBUG = -Wall -Wextra -g -O0 # Debug configuration flags
LDFLAGS = -static -mwindows -lgdiplus -lstdc++ # For Windows GUI applications

# Source files
SOURCES = dpsgirl.cpp

# Include directories (if any)
INCLUDE_DIRS = -I. # Add any necessary include paths

# Resource file
RC_FILE = dpsgirl.rc
RES_OBJ = dpsgirl.res # Compiled resource object
WINDRES = x86_64-w64-mingw32-windres # Use the MinGW resource compiler

# Output executable name
TARGET = dpsgirl.exe

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default rule: Build the release version
all: release

# Build the release version
release: $(TARGET)

# Build the debug version
debug:
	$(CC) $(CFLAGS_DEBUG) $(INCLUDE_DIRS) -c $(SOURCES)
	$(RES_OBJ): $(RC_FILE)
		$(WINDRES) $(RC_FILE) -o $(RES_OBJ)
	$(CC) $(CFLAGS_DEBUG) $(INCLUDE_DIRS) $(OBJECTS) $(RES_OBJ) $(LDFLAGS) -o $(TARGET) -g

# Rule to link the object files and resources to create the executable
$(TARGET): $(OBJECTS) $(RES_OBJ)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $(OBJECTS) $(RES_OBJ) $(LDFLAGS) -o $(TARGET)

# Rule to compile a single .cpp file to a .o object file
%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# Rule to compile the resource file
$(RES_OBJ): $(RC_FILE)
	$(WINDRES) --output-format=coff $(RC_FILE) -o $(RES_OBJ)

# Clean rule: Remove generated files
clean:
	rm -f $(TARGET) $(OBJECTS) $(RES_OBJ) *.o *.res *.pdb

.PHONY: all release debug clean

contours:
	g++ ./contours.cpp -Wall -o contours.bin $(pkg-config --libs --cflags opencv4) # && ./contours.bin >> dpsgirl.cpp
