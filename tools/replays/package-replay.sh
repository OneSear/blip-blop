#!/bin/sh
# Plays regression replays with the game as installed from a release package
# (migration.md, definition of done: the campaign completed from the release
# packages) and compares the fingerprints with tests/replays/expected/.
# The game runs from an empty directory, so it finds its data the way a
# player's copy does (next to the executable; macOS: Contents/Resources).
#
#   tools/replays/package-replay.sh PACKAGE [REPLAY...]
#
# PACKAGE: the Linux .tar.gz, the macOS .dmg or the Windows .zip made by
# cpack. REPLAY: names in tests/replays (default: campaign smoke).
set -e
root=$(cd "$(dirname "$0")/../.." && pwd)
package=$1
shift
[ -f "$package" ] || { echo "usage: $0 PACKAGE [REPLAY...]" >&2; exit 2; }
[ $# -gt 0 ] || set -- campaign smoke

# Windows (Git Bash): cmake is a native program, give it C:/... paths.
native() {
    if command -v cygpath > /dev/null; then cygpath -m "$1"; else printf '%s\n' "$1"; fi
}
package=$(native "$(cd "$(dirname "$package")" && pwd)/$(basename "$package")")
work=$(mktemp -d)
mount=""
cleanup() {
    [ -z "$mount" ] || hdiutil detach -quiet "$mount" || true
    rm -rf "$work"
}
trap cleanup EXIT
mkdir "$work/run" "$work/out" "$work/install"

case "$package" in
    *.tar.gz)
        tar -xzf "$package" -C "$work/install"
        game=$(find "$work/install" -type f -name blipblop | head -n 1) ;;
    *.zip)
        (cd "$work/install" && cmake -E tar xf "$package")
        game=$(find "$work/install" -type f -name blipblop.exe | head -n 1) ;;
    *.dmg)
        mount="$work/mount"
        hdiutil attach -quiet -nobrowse -readonly -mountpoint "$mount" "$package"
        game="$mount/Blip and Blop.app/Contents/MacOS/blipblop" ;;
    *)
        echo "$0: unknown package type: $package" >&2
        exit 2 ;;
esac
[ -n "$game" ] && [ -f "$game" ] || { echo "$0: no game executable in $package" >&2; exit 1; }
echo "Game: $game"

game=$(native "$game")
root=$(native "$root")
work=$(native "$work")

status=0
for name in "$@"; do
    cmake -DGAME="$game" \
          -DREPLAY="$root/tests/replays/$name.bbreplay" \
          -DEXPECTED="$root/tests/replays/expected/$name.bbhash" \
          -DOUTPUT="$work/out/$name.bbhash" \
          -DRUN_DIR="$work/run" \
          -P "$root/tests/replays/run_replay.cmake" || status=1
done
exit $status
