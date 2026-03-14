#!/usr/bin/env bash
set -euo pipefail

# Open an interactive shell in the dev container.
# Usage:
#   ./scripts/container-shell.sh
#   ./scripts/container-shell.sh root   # open shell as root
#   ./scripts/container-shell.sh -T     # no TTY (for CI/debug)
#
# Optional env vars:
#   COMPOSE_FILE   (default: compose.yml)
#   COMPOSE_PROJECT_NAME (optional)
#   DBAT_SERVICE   (default: dbat-dev)
#   DBAT_SHELL     (default: bash)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${PROJECT_ROOT}"

COMPOSE_FILE="${COMPOSE_FILE:-compose.yml}"
DBAT_SERVICE="${DBAT_SERVICE:-dbat-dev}"
DBAT_SHELL="${DBAT_SHELL:-bash}"

if [[ ! -f "${COMPOSE_FILE}" ]]; then
  echo "Error: ${COMPOSE_FILE} not found in ${PROJECT_ROOT}" >&2
  exit 1
fi

# Compose command wrapper for consistency and easy override.
compose() {
  docker compose -f "${COMPOSE_FILE}" "$@"
}

# Ensure Docker is reachable.
if ! docker info >/dev/null 2>&1; then
  echo "Error: Docker daemon is not reachable. Start Docker and try again." >&2
  exit 1
fi

# Start service container in background if not already running.
compose up -d "${DBAT_SERVICE}"

# Parse simple invocation modes.
# - "root": open as root user
# - pass-through flags before shell command (e.g., -T)
USER_FLAG=()
TTY_FLAG=()
CMD=("${DBAT_SHELL}")

for arg in "$@"; do
  case "${arg}" in
    root)
      USER_FLAG=(--user root)
      ;;
    -T)
      TTY_FLAG=(-T)
      ;;
    *)
      CMD=("${arg}")
      ;;
  esac
done

# If a shell command/name was provided as non-flag arg, run it; else default shell.
compose exec "${TTY_FLAG[@]}" "${USER_FLAG[@]}" "${DBAT_SERVICE}" "${CMD[@]}"
