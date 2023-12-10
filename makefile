CC = /usr/bin/gcc-11
CFLAGS = -Wall -g -O2 -Werror -std=gnu99 -Wno-unused-function -fcommon

BIN_HARDWARE = ./bin/test_hardware

SRC_DIR = ./src

# debug
COMMON = $(SRC_DIR)/common/print.c $(SRC_DIR)/common/convert.c

# hardware
CPU =$(SRC_DIR)/hardware/cpu/mmu.c $(SRC_DIR)/hardware/cpu/isa.c
MEMORY = $(SRC_DIR)/hardware/memory/dram.c

# main
TEST_HARDWARE = $(SRC_DIR)/tests/test_hardware.c

.PHONY:hardware
hardware:
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(COMMON) $(CPU) $(MEMORY) $(DISK) $(TEST_HARDWARE) -o $(BIN_HARDWARE)
	./$(BIN_HARDWARE)

.PHONY:link
link:
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(COMMON)  $(SRC_DIR)/tests/test_elf.c  $(SRC_DIR)/linker/parseElf.c  $(SRC_DIR)/linker/staticlink.c -o ./bin/test_elf
	./bin/test_elf

clean:
	rm -f *.o *~ $(BIN_HARDWARE)