#!/usr/bin/env bash
# common.sh -- shared helpers for kbox scripts and git hooks.

set_colors()
{
    local default_color
    default_color=$(git config --get color.ui 2> /dev/null || echo 'auto')
    if [[ "$default_color" == "always" ]] || [[ "$default_color" == "auto" && -t 1 ]]; then
        RED='\033[1;31m'
        GREEN='\033[1;32m'
        YELLOW='\033[1;33m'
        BLUE='\033[1;34m'
        WHITE='\033[1;37m'
        CYAN='\033[1;36m'
        NC='\033[0m'
    else
        RED=''
        GREEN=''
        YELLOW=''
        BLUE=''
        WHITE=''
        CYAN=''
        NC=''
    fi
}

# Print a fatal error message and exit.
throw()
{
    local fmt="$1"
    shift
    printf "\n${RED}[!] $fmt${NC}\n" "$@" >&2
    exit 1
}

# Skip hooks entirely when running in CI (GitHub Actions, etc.).
check_ci()
{
    if [ -n "${CI:-}" ] || [ -d "/home/runner/work" ]; then
        exit 0
    fi
}
