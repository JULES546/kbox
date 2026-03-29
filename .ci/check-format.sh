#!/usr/bin/env bash

# Verify clang-format-20 conformance for all tracked C/H files.

# Require clang-format-20 -- older versions produce different output.
if [ -z "${CLANG_FORMAT:-}" ]; then
    if command -v clang-format-20 >/dev/null 2>&1; then
        CLANG_FORMAT="clang-format-20"
    else
        echo "Error: clang-format-20 is required (older versions differ in style)" >&2
        exit 1
    fi
fi

ret=0
while IFS= read -r -d '' file; do
    expected=$(mktemp)
    "$CLANG_FORMAT" "$file" >"$expected" 2>/dev/null
    if ! diff -u -p --label="$file" --label="expected coding style" "$file" "$expected"; then
        ret=1
    fi
    rm -f "$expected"
done < <(git ls-files -z -- '*.c' '*.h')

exit $ret
