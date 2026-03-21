# mk/tests.mk - Test targets (unit, integration, stress, guest binaries)

# Unit test files (no LKL dependency)
TEST_DIR   = tests/unit
TEST_SRCS  = $(TEST_DIR)/test-runner.c \
             $(TEST_DIR)/test-fd-table.c \
             $(TEST_DIR)/test-path.c \
             $(TEST_DIR)/test-identity.c \
             $(TEST_DIR)/test-syscall-nr.c \
             $(TEST_DIR)/test-elf.c

# Unit tests link only the pure-computation sources (no LKL)
TEST_SUPPORT_SRCS = $(SRC_DIR)/fd-table.c \
                    $(SRC_DIR)/path.c \
                    $(SRC_DIR)/identity.c \
                    $(SRC_DIR)/syscall-nr.c \
                    $(SRC_DIR)/elf.c

TEST_TARGET  = tests/unit/test-runner

# Guest test programs (compiled statically, run inside kbox)
GUEST_DIR    = tests/guest
GUEST_SRCS   = $(wildcard $(GUEST_DIR)/*-test.c)
GUEST_BINS   = $(GUEST_SRCS:.c=)

# Stress test programs (compiled statically, run inside kbox)
STRESS_DIR   = tests/stress
STRESS_SRCS  = $(wildcard $(STRESS_DIR)/*.c)
STRESS_BINS  = $(STRESS_SRCS:.c=)

# Rootfs image
ROOTFS       = alpine.ext4

# ---- Test targets ----

check: check-unit check-integration check-stress

check-unit: $(TEST_TARGET)
	@echo "  RUN     check-unit"
	$(Q)./$(TEST_TARGET)

# Unit tests are built WITHOUT linking LKL.
# We define LKL stubs for functions referenced by test support code.
$(TEST_TARGET): $(TEST_SRCS) $(TEST_SUPPORT_SRCS) $(wildcard .config)
	@echo "  LD      $@"
	$(Q)$(CC) $(CFLAGS) -DKBOX_UNIT_TEST -o $@ $(TEST_SRCS) $(TEST_SUPPORT_SRCS) $(LDFLAGS)

check-integration: $(TARGET) guest-bins stress-bins $(ROOTFS)
	@echo "  RUN     check-integration"
	$(Q)./scripts/run-tests.sh ./$(TARGET) $(ROOTFS)

check-stress: $(TARGET) stress-bins $(ROOTFS)
	@echo "  RUN     check-stress"
	$(Q)./scripts/run-stress.sh ./$(TARGET) $(ROOTFS) || \
	  echo "(stress test failures are non-blocking -- see TODO.md)"

# ---- Guest / stress binaries (static, no ASAN) ----
# These are cross-compiled on Linux and placed into the rootfs.
# They must be statically linked and cannot use sanitizers.

guest-bins: $(GUEST_BINS)

$(GUEST_DIR)/%-test: $(GUEST_DIR)/%-test.c
	@echo "  CC      $<"
	$(Q)$(CC) -std=gnu11 -Wall -Wextra -O2 -static -o $@ $<

stress-bins: $(STRESS_BINS)

$(STRESS_DIR)/%: $(STRESS_DIR)/%.c
	@echo "  CC      $<"
	$(Q)$(CC) -std=gnu11 -Wall -Wextra -O2 -static -pthread -o $@ $<

# ---- Rootfs ----

rootfs: $(ROOTFS)

$(ROOTFS): scripts/mkrootfs.sh scripts/alpine-sha256.txt $(GUEST_BINS) $(STRESS_BINS)
	@echo "  GEN     $@"
	$(Q)ALPINE_ARCH=$(ARCH) ./scripts/mkrootfs.sh

.PHONY: check check-unit check-integration check-stress guest-bins stress-bins rootfs
