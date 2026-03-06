# MatryoshkaOS - Integration Test Runner
#
# Requires: QEMU, Python 3
# The ISO must be built first (make iso from project root).

COLOR_G := \033[32m
COLOR_R := \033[31m
COLOR_B := \033[1;34m
COLOR_0 := \033[0m

TESTS := test_boot test_shell test_tasks

.PHONY: test $(TESTS)

test: $(TESTS)
	@echo ""
	@echo "$(COLOR_G)✓ All integration tests passed$(COLOR_0)"

test_boot:
	@python3 test_boot.py

test_shell:
	@python3 test_shell.py

test_tasks:
	@python3 test_tasks.py
