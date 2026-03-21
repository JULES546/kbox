# mk/format.mk - Code formatting rules

CLANG_FORMAT := $(shell command -v clang-format-20 2>/dev/null || \
                        command -v clang-format 2>/dev/null)
SHFMT        := shfmt
BLACK        := black

C_SRCS_FMT  := $(wildcard src/*.c src/*.h include/kbox/*.h \
                tests/unit/*.c tests/unit/*.h \
                tests/guest/*.c tests/stress/*.c)
SH_SRCS_FMT := $(wildcard scripts/*.sh)
PY_SRCS_FMT := $(wildcard scripts/gdb/*.py)

indent:
ifeq ($(CLANG_FORMAT),)
	$(error clang-format not found; install clang-format or clang-format-20)
endif
	$(Q)$(CLANG_FORMAT) --version | grep -q 'version 20' || \
	  { echo "error: clang-format version 20 required"; exit 1; }
	@echo "  INDENT  C sources"
	$(Q)$(CLANG_FORMAT) -i $(C_SRCS_FMT)
	@echo "  INDENT  shell scripts"
	$(Q)$(SHFMT) -i 4 -fn -ci -sr -bn -w $(SH_SRCS_FMT)
	@echo "  INDENT  Python scripts"
	$(Q)$(BLACK) -q $(PY_SRCS_FMT)

.PHONY: indent
