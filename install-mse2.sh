#!/usr/bin/env bash
set -Eeuo pipefail

readonly INSTALL_DIR='/opt/MagicSetEditor2'
readonly INSTALL_BINARY="${INSTALL_DIR}/magicseteditor"
readonly LINK_PATH='/usr/local/bin/magicseteditor'

usage() {
  printf 'Usage: %s <build-directory>\n' "$(basename "$0")"
  printf 'Example: %s build-ninja\n' "$(basename "$0")"
}

if [[ $# -ne 1 ]]; then
  usage >&2
  exit 2
fi

build_dir=$1
source_binary="${build_dir%/}/magicseteditor"

if [[ ! -f "$source_binary" ]]; then
  printf 'Error: executable not found: %s\n' "$source_binary" >&2
  printf 'Build first with: cmake --build "%s" --parallel\n' "$build_dir" >&2
  exit 1
fi

if [[ ! -x "$source_binary" ]]; then
  printf 'Error: file is not executable: %s\n' "$source_binary" >&2
  exit 1
fi

if [[ -d "$LINK_PATH" && ! -L "$LINK_PATH" ]]; then
  printf 'Error: %s is a directory; refusing to overwrite it.\n' "$LINK_PATH" >&2
  exit 1
fi

if [[ $(id -u) -eq 0 ]]; then
  privileged=()
else
  if ! command -v sudo >/dev/null 2>&1; then
    printf 'Error: sudo is required to install to /opt and /usr/local/bin.\n' >&2
    exit 1
  fi
  privileged=(sudo)
fi

printf 'Installing %s to %s\n' "$source_binary" "$INSTALL_BINARY"
"${privileged[@]}" install -d -m 755 "$INSTALL_DIR"
"${privileged[@]}" install -m 755 "$source_binary" "$INSTALL_BINARY"
"${privileged[@]}" install -d -m 755 /usr/local/bin
"${privileged[@]}" ln -sfnT "$INSTALL_BINARY" "$LINK_PATH"

printf '\nInstallation complete.\n'
printf 'Binary:  %s\n' "$INSTALL_BINARY"
printf 'Command: %s\n' "$LINK_PATH"
printf 'Run:     magicseteditor\n'
