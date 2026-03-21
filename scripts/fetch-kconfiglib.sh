#!/bin/bash
# Fetch Kconfiglib into tools/kconfig/ if not already present.
# Shallow-clones the official repo to avoid pulling full history.

set -euo pipefail

KCONFIG_DIR="tools/kconfig"
KCONFIG_REPO="https://github.com/sysprog21/Kconfiglib.git"
KCONFIG_REF="4dbd20b5193a"

# Re-clone if present but pinned to wrong commit.
if [ -f "$KCONFIG_DIR/menuconfig.py" ]; then
    CURRENT_REF=$(git -C "$KCONFIG_DIR" rev-parse --short=12 HEAD 2> /dev/null || true)
    if [ "$CURRENT_REF" = "$KCONFIG_REF" ]; then
        exit 0
    fi
    echo "Kconfiglib version mismatch ($CURRENT_REF != $KCONFIG_REF), re-fetching..."
    rm -rf "$KCONFIG_DIR"
fi

echo "Fetching Kconfiglib ($KCONFIG_REF)..."
rm -rf "$KCONFIG_DIR"
mkdir -p "$(dirname "$KCONFIG_DIR")"
git clone "$KCONFIG_REPO" "$KCONFIG_DIR"
git -C "$KCONFIG_DIR" checkout "$KCONFIG_REF"
echo "Kconfiglib installed at $KCONFIG_DIR (pinned to $KCONFIG_REF)"
