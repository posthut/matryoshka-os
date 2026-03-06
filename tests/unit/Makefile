# MatryoshkaOS - Unit Test Build
#
# Compiles kernel source files for the host and links with test harnesses.
# Run:  make test        (build + run all)
#       make test_string  (build + run one suite)

KERNEL   := ../../kernel
INCLUDES := -I$(KERNEL)/include -I.
CC       := gcc
CFLAGS   := -Wall -Wextra -Wno-unused-parameter -std=c11 -g -O0 \
            -D_GNU_SOURCE -fno-builtin $(INCLUDES)

BUILD    := build
TESTS    := test_string test_heap test_pmm test_task

COLOR_G  := \033[32m
COLOR_R  := \033[31m
COLOR_B  := \033[1;34m
COLOR_0  := \033[0m

.PHONY: test clean $(TESTS)

test: $(TESTS)
	@echo ""
	@echo "$(COLOR_B)╔══════════════════════════════════════╗$(COLOR_0)"
	@echo "$(COLOR_B)║    Running all unit test suites      ║$(COLOR_0)"
	@echo "$(COLOR_B)╚══════════════════════════════════════╝$(COLOR_0)"
	@fail=0; \
	for t in $(TESTS); do \
		./$(BUILD)/$$t || fail=1; \
	done; \
	echo ""; \
	if [ $$fail -eq 0 ]; then \
		echo "$(COLOR_G)✓ All unit test suites passed$(COLOR_0)"; \
	else \
		echo "$(COLOR_R)✗ Some test suites failed$(COLOR_0)"; \
		exit 1; \
	fi

$(BUILD):
	@mkdir -p $(BUILD)

# ── Kernel objects (compiled for host) ─────────────────────────────

$(BUILD)/string.o: $(KERNEL)/string.c | $(BUILD)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/heap.o: $(KERNEL)/mm/heap.c | $(BUILD)
	@$(CC) $(CFLAGS) -include compat.h -c $< -o $@

$(BUILD)/pmm.o: $(KERNEL)/mm/pmm.c | $(BUILD)
	@$(CC) $(CFLAGS) -include compat.h -c $< -o $@

$(BUILD)/multiboot2.o: $(KERNEL)/arch/x86_64/multiboot2.c | $(BUILD)
	@$(CC) $(CFLAGS) -include compat.h -c $< -o $@

$(BUILD)/task.o: $(KERNEL)/proc/task.c | $(BUILD)
	@$(CC) $(CFLAGS) -include compat.h -c $< -o $@

$(BUILD)/mocks.o: mocks.c | $(BUILD)
	@$(CC) $(CFLAGS) -include compat.h -c $< -o $@

# ── Test binaries ──────────────────────────────────────────────────

test_string: $(BUILD)/test_string
$(BUILD)/test_string: test_string.c $(BUILD)/string.o | $(BUILD)
	@$(CC) $(CFLAGS) -include compat.h $^ -o $@

test_heap: $(BUILD)/test_heap
$(BUILD)/test_heap: test_heap.c $(BUILD)/heap.o $(BUILD)/string.o $(BUILD)/mocks.o | $(BUILD)
	@$(CC) $(CFLAGS) -include compat.h $^ -o $@

test_pmm: $(BUILD)/test_pmm
$(BUILD)/test_pmm: test_pmm.c $(BUILD)/pmm.o $(BUILD)/multiboot2.o $(BUILD)/mocks.o | $(BUILD)
	@$(CC) $(CFLAGS) -include compat.h $^ -Wl,--defsym,_kernel_end=0x400000 -o $@

test_task: $(BUILD)/test_task
$(BUILD)/test_task: test_task.c $(BUILD)/task.o $(BUILD)/mocks.o $(BUILD)/string.o | $(BUILD)
	@$(CC) $(CFLAGS) -include compat.h $^ -o $@

clean:
	@rm -rf $(BUILD)
