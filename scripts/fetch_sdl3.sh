#!/bin/bash
set -e

# --- Config: Releases and download URLs ---
SDL3_VER=3.2.24
SDL3_TTF_VER=3.2.2
declare -A RELEASES
RELEASES["SDL3_v$SDL3_VER"]="https://github.com/libsdl-org/SDL/releases/download/release-3.2.24/SDL3-devel-$SDL3_VER-VC.zip"
RELEASES["SDL3_ttf_v$SDL3_TTF_VER"]="https://github.com/libsdl-org/SDL_ttf/releases/download/release-3.2.2/SDL3_ttf-devel-3.2.2-VC.zip"

# --- Config: Destinations (space-separated strings) ---
declare -A DESTS
DESTS["SDL3_v$SDL3_VER"]="lib/SDL3 lib/UI/lib/SDL3"
DESTS["SDL3_ttf_v$SDL3_TTF_VER"]="lib/SDL3_ttf"

# Temporary files/folders
TMP_ZIP="tmp.zip"
TMP_DIR="tmp_extract"
BASE_DIR="external"

mkdir -p "$BASE_DIR"

for NAME in "${!RELEASES[@]}"; do
    URL="${RELEASES[$NAME]}"
    echo "[$NAME] Downloading..."
    curl -sL -o "$TMP_ZIP" "$URL"
    echo "[$NAME] Unzipping..."
    mkdir -p "$TMP_DIR"
    unzip -oq "$TMP_ZIP" -d "$TMP_DIR"

    # Convert space-separated string to array
    read -r -a DEST_ARRAY <<< "${DESTS[$NAME]}"
    
    SRC_DIR=$(find "$TMP_DIR" -mindepth 1 -maxdepth 1 -type d)
    
    # Copy to other destinations
    for DEST in "${DEST_ARRAY[@]:0}"; do
        echo "[$NAME] Copying to $DEST..."
        mkdir -p "$(dirname "$DEST")"
        rm -rf "$DEST"
        cp -rf "$SRC_DIR" "$DEST"
    done

    # Clean up temporary files
    rm -rf "$TMP_ZIP" "$TMP_DIR"
done