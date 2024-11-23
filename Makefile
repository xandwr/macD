CC = gcc-10
CFLAGS = -Wall -Iinclude

# Directories
SRC_DIR = src
BIN_DIR = bin
BUILD_DIR = build

PROGRAM_SRC_DIR = programs/src
PROGRAM_BIN_DIR = programs/bin
PROGRAM_BUILD_DIR = programs/build

# macD sources, objects, and executables
MACD_SRC = $(SRC_DIR)/macD.c
MACD_OBJ = $(BIN_DIR)/macD.o
MACD_EXEC = $(BUILD_DIR)/macD

# Program sources, objects, and executables
PROGRAM_SRC = $(wildcard $(PROGRAM_SRC_DIR)/*.c)
PROGRAM_OBJ = $(patsubst $(PROGRAM_SRC_DIR)/%.c, $(PROGRAM_BIN_DIR)/%.o, $(PROGRAM_SRC))
PROGRAM_EXEC = $(patsubst $(PROGRAM_SRC_DIR)/%.c, $(PROGRAM_BUILD_DIR)/%, $(PROGRAM_SRC))

# Targets
all: $(MACD_EXEC) $(PROGRAM_EXEC)

# macD executable
$(MACD_EXEC): $(MACD_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(MACD_OBJ) -o $@

# Program executables
$(PROGRAM_BUILD_DIR)/%: $(PROGRAM_BIN_DIR)/%.o
	@mkdir -p $(PROGRAM_BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@ -lm

# Compile macD.c
$(MACD_OBJ): $(MACD_SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile program source files
$(PROGRAM_BIN_DIR)/%.o: $(PROGRAM_SRC_DIR)/%.c
	@mkdir -p $(PROGRAM_BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR) $(PROGRAM_BIN_DIR) $(PROGRAM_BUILD_DIR)
