#!/usr/bin/env bash

# Run cppcheck static analysis on kbox source files (src/ only).
# Tests are excluded -- they use stubs and have different rules.
#
# CI mode: --max-configs=1 + --enable=warning for speed.
# Deep analysis runs in the pre-commit hook on individual changed files.

set -e -u -o pipefail

# Exclude syscall-nr.c: it's a data table that causes cppcheck to hang
# on older versions (>100s). The pre-commit hook checks it per-file.
mapfile -t SOURCES < <(git ls-files -z -- 'src/*.c' | tr '\0' '\n' | grep -v 'syscall-nr\.c')

if [ ${#SOURCES[@]} -eq 0 ]; then
    echo "No tracked C source files found."
    exit 0
fi

# Run cppcheck with a timeout to prevent CI hangs.
# 120s is generous -- this should finish in <30s with --max-configs=1.
timeout 120 cppcheck \
    -I. -Iinclude -Isrc \
    --platform=unix64 \
    --enable=warning \
    --max-configs=1 --error-exitcode=1 --inline-suppr \
    --suppress=checkersReport --suppress=unmatchedSuppression \
    --suppress=missingIncludeSystem --suppress=noValidConfiguration \
    --suppress=normalCheckLevelMaxBranches \
    --suppress=preprocessorErrorDirective \
    -D_GNU_SOURCE -D__linux__ \
    "${SOURCES[@]}"
