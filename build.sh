#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

STS_DIR="${STS_DIR:-$ROOT/game}"
LANGS="${LANGS:-eng}"
AUDIO_KBPS="${AUDIO_KBPS:-0}"
MODE="${BUILD_MODE:-slim}"
CLEAN=0

usage() {
    cat <<'USAGE'
Usage: ./build.sh [options]

Options:
  --sts-dir PATH      Folder containing desktop-1.0.jar
  --langs LIST        Comma-separated languages to keep, default: eng
  --audio-kbps N      Re-encode OGG audio at N kbps, default: 0 (copy)
  --slim              Build code-only VPK plus external assets, default
  --full              Bundle assets into the VPK
  --clean             Remove previous generated output before building
  -h, --help          Show this help

Environment variables are also supported:
  STS_DIR, LANGS, AUDIO_KBPS, BUILD_MODE
USAGE
}

die() {
    echo "error: $*" >&2
    exit 2
}

need_command() {
    command -v "$1" >/dev/null 2>&1 || die "$1 not found on PATH"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --sts-dir)
            [[ $# -ge 2 ]] || die "--sts-dir needs a path"
            STS_DIR="$2"
            shift 2
            ;;
        --langs)
            [[ $# -ge 2 ]] || die "--langs needs a value"
            LANGS="$2"
            shift 2
            ;;
        --audio-kbps)
            [[ $# -ge 2 ]] || die "--audio-kbps needs a value"
            AUDIO_KBPS="$2"
            shift 2
            ;;
        --slim)
            MODE="slim"
            shift
            ;;
        --full)
            MODE="full"
            shift
            ;;
        --clean)
            CLEAN=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
done

[[ "$MODE" == "slim" || "$MODE" == "full" ]] || die "BUILD_MODE must be slim or full"
[[ -f "$STS_DIR/desktop-1.0.jar" ]] || die "$STS_DIR/desktop-1.0.jar not found"
[[ -n "${VITASDK:-}" ]] || die "VITASDK is not set"

need_command python3
need_command gcc
need_command gradle
need_command cmake
need_command make
need_command java
if ! command -v magick >/dev/null 2>&1 && ! command -v convert >/dev/null 2>&1; then
    die "ImageMagick not found on PATH (need magick or convert)"
fi

if [[ "$AUDIO_KBPS" != "0" ]]; then
    need_command ffmpeg
fi

target="vpk-slim"
vpk="build/STSVITA-slim.vpk"
if [[ "$MODE" == "full" ]]; then
    target="vpk"
    vpk="build/STSVITA.vpk"
fi

if [[ "$CLEAN" == "1" ]]; then
    make clean
fi

echo "Build mode: $MODE"
echo "Pipeline: verify -> extract -> assets -> livearea -> transpile -> native"
echo "Command: make $target STS_DIR=$STS_DIR LANGS=$LANGS AUDIO_KBPS=$AUDIO_KBPS"
echo

make "$target" STS_DIR="$STS_DIR" LANGS="$LANGS" AUDIO_KBPS="$AUDIO_KBPS"

[[ -f "$vpk" ]] || die "build completed without producing $vpk"
[[ -d work/assets_vita ]] || die "build completed without producing work/assets_vita"

asset_count="$(find work/assets_vita -type f | wc -l | tr -d '[:space:]')"
dds_count="$(find work/assets_vita -type f -name '*.dds' | wc -l | tr -d '[:space:]')"
fallback_count="$(find work/assets_vita -type f \( -name '*.png' -o -name '*.jpg' \) | wc -l | tr -d '[:space:]')"
audio_count="$(find work/assets_vita -type f \( -name '*.ogg' -o -name '*.wav' \) | wc -l | tr -d '[:space:]')"

echo
echo "Build complete."
echo "VPK: $vpk"
echo "Assets: work/assets_vita ($asset_count files)"
echo "Textures: $dds_count DDS, $fallback_count PNG/JPG fallbacks"
echo "Audio: $audio_count OGG/WAV files"
if [[ "$MODE" == "slim" ]]; then
    echo "Copy work/assets_vita contents to ux0:data/stsvita/assets/ on the Vita."
fi
