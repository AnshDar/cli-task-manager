# Build automation for the CLI Task Manager.

CXX      := g++
# -static produces a self-contained executable. On MSYS2/MinGW this also
# avoids loading mismatched libstdc++/libgcc DLLs from a different toolchain
# that happens to appear earlier on PATH.
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
LDFLAGS  := -static
SRC_DIR  := src
SOURCES  := $(SRC_DIR)/main.cpp \
            $(SRC_DIR)/task.cpp \
            $(SRC_DIR)/taskmanager.cpp \
            $(SRC_DIR)/storage.cpp
TARGET   := task_manager

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe
