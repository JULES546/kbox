#!/usr/bin/env bash

# Security checks for kbox source files (src/ and include/ only).
# Tests and guest/stress binaries are excluded -- they have different rules.
#
# 1. Banned functions -- unsafe libc calls with safer alternatives.
# 2. Credential / secret patterns -- catch accidental key leaks.
# 3. Dangerous preprocessor -- detect disabled security features.

set -u -o pipefail

failed=0

# --- Patterns ---
banned='(^|[^[:alnum:]_])(gets|sprintf|vsprintf|strcpy|stpcpy|strcat|atoi|atol|atoll|atof|mktemp|tmpnam|tempnam)[[:space:]]*\('
secrets='(password|secret|api_key|private_key|token)[[:space:]]*=[[:space:]]*"[^"]+'
dangerous_pp='#[[:space:]]*(undef|define)[[:space:]]+((_FORTIFY_SOURCE[[:space:]]+0)|(__SSP__))'
comment_only='^[[:space:]]*(//|/\*|\*|\*/)'

# Only scan kbox source, not tests/guest/stress.
while IFS= read -r -d '' f; do
    # Skip comment-only matches before checking.
    code=$(grep -vE "$comment_only" "$f")

    if echo "$code" | grep -qE "$banned"; then
        echo "Banned function in $f:"
        grep -nE "$banned" "$f" | grep -vE "$comment_only"
        failed=1
    fi
    if echo "$code" | grep -iqE "$secrets"; then
        echo "Possible hardcoded secret in $f:"
        grep -inE "$secrets" "$f" | grep -vE "$comment_only"
        failed=1
    fi
    if echo "$code" | grep -qE "$dangerous_pp"; then
        echo "Dangerous preprocessor directive in $f:"
        grep -nE "$dangerous_pp" "$f" | grep -vE "$comment_only"
        failed=1
    fi
done < <(git ls-files -z -- 'src/*.c' 'src/*.h' 'include/*.h' 'include/**/*.h')

if [ $failed -eq 0 ]; then
    echo "Security checks passed."
fi

exit $failed
