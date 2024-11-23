CC = gcc-10
CFLAGS = -Wall -Iinclude

SRC_DIR = src
BIN_DIR = bin
INCLUDE_DIR = include
BUILD_DIR = ./

MACD_SRC = $(SRC_DIR)/macD.c
TEST_PROGRAM_SRC = $(SRC_DIR)/test_program.c
MACD_OBJ = $(BIN_DIR)/macD.o
TEST_PROGRAM_OBJ = $(BIN_DIR)/test_program.o
EXECUTABLE_MACD = $(BUILD_DIR)/macD
EXECUTABLE_TEST = $(BUILD_DIR)/test_program

# Targets
all: $(EXECUTABLE_MACD) $(EXECUTABLE_TEST)

# macD executable
$(EXECUTABLE_MACD): $(MACD_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(MACD_OBJ) -o $@

# test_program executable
$(EXECUTABLE_TEST): $(TEST_PROGRAM_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TEST_PROGRAM_OBJ) -o $@

# Compile macD.c
$(MACD_OBJ): $(MACD_SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile test_program.c
$(TEST_PROGRAM_OBJ): $(TEST_PROGRAM_SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR)