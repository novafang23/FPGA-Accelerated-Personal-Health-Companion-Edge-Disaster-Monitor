# SIH26181 Health Companion - Makefile
# Builds the PC simulation and embedded targets

CC      ?= gcc
CFLAGS  := -std=c11 -Wall -Wextra -Werror -pedantic -O2 -g
CFLAGS  += -I.
CFLAGS  += -D_GNU_SOURCE

# Platform-specific settings
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LDFLAGS += -lm -lrt
    CFLAGS  += -D_POSIX_C_SOURCE=200809L
endif
ifeq ($(UNAME_S),Darwin)
    LDFLAGS += -lm
endif
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lm
    CFLAGS  += -D_WIN32
endif

# Source files
CORE_DIR    := ../core
CFLAGS      += -I. -I$(CORE_DIR)

SRCS_CORE := \
    $(CORE_DIR)/hrv_analysis.c \
    $(CORE_DIR)/spo2_engine.c \
    $(CORE_DIR)/disaster_risk_engine.c \
    $(CORE_DIR)/nn_risk_model.c \
    $(CORE_DIR)/nn_risk_model_int8.c

SRCS_DRIVERS := \
    i2c_hal.c \
    max30102.c \
    bme280.c \
    pms5003.c \
    driver_ppg.c \
    ssd1306.c

SRCS_COMMON := $(SRCS_CORE) $(SRCS_DRIVERS)

SRCS_SIM := main_simulation.c
SRCS_COMPARE := compare_harness.c
SRCS_TEST := test_disaster_risk_engine.c

# Targets
TARGETS := health_demo compare_harness test_engine

.PHONY: all clean test

all: $(TARGETS)

# Main simulation demo
health_demo: $(SRCS_COMMON) $(SRCS_SIM)
	$(CC) $(CFLAGS) -o $@ $(SRCS_COMMON) $(SRCS_SIM) $(LDFLAGS)

# Comparison harness
compare_harness: $(SRCS_COMMON) $(SRCS_COMPARE)
	$(CC) $(CFLAGS) -o $@ $(SRCS_COMMON) $(SRCS_COMPARE) $(LDFLAGS)

# Unit tests
test_engine: $(SRCS_COMMON) $(SRCS_TEST)
	$(CC) $(CFLAGS) -o $@ $(SRCS_COMMON) $(SRCS_TEST) $(LDFLAGS)

# Run tests
test: test_engine
	./test_engine

# Static analysis (requires cppcheck)
analyze:
	cppcheck --enable=all --std=c11 --suppress=missingIncludeSystem $(SRCS_COMMON) $(SRCS_SIM) $(SRCS_COMPARE) $(SRCS_TEST)

# Format check (requires clang-format)
format-check:
	clang-format --dry-run --Werror *.c *.h

# Format (requires clang-format)
format:
	clang-format -i *.c *.h

# Clean
clean:
	rm -f $(TARGETS) *.exe *.o

# Embedded build for Zynq (bare-metal)
# Usage: make embedded CROSS_COMPILE=arm-none-eabi-
embedded: CFLAGS += -DZYNQ_HW -ffreestanding -nostdlib
embedded: CC = $(CROSS_COMPILE)gcc
embedded: $(SRCS_COMMON)
	$(CC) $(CFLAGS) -c $(SRCS_COMMON)

# Help
help:
	@echo "Available targets:"
	@echo "  all           - Build all targets (default)"
	@echo "  health_demo   - Build main simulation"
	@echo "  compare_harness - Build comparison harness"
	@echo "  test_engine   - Build and run unit tests"
	@echo "  test          - Run unit tests"
	@echo "  analyze       - Run static analysis (cppcheck)"
	@echo "  format        - Format code (clang-format)"
	@echo "  format-check  - Check formatting"
	@echo "  clean         - Remove build artifacts"
	@echo "  embedded      - Cross-compile for Zynq (set CROSS_COMPILE)"
	@echo ""
	@echo "Variables:"
	@echo "  CC            - C compiler (default: gcc)"
	@echo "  CFLAGS        - Compiler flags"
	@echo "  CROSS_COMPILE - Prefix for cross-compiler (e.g., arm-none-eabi-)"