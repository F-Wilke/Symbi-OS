#!/bin/bash
# add_kotbl_section.sh - Add a .kotbl section to an ELF file
#
# Usage: ./add_kotbl_section.sh <elf_file> <data_file>
#
# The .kotbl section is marked as readonly, debug, and contents (not allocated/loaded)

set -e

if [ $# -lt 1 ]; then
    echo "Usage: $0 <elf_file> [data_file]"
    echo ""
    echo "Adds a .kotbl section to the ELF file from the specified data file."
    echo "If no data_file is specified, uses kotbl.bin in the current directory."
    echo ""
    exit 1
fi

ELF_FILE="$1"
DATA_FILE="${2:-kotbl.bin}"

if [ ! -f "$ELF_FILE" ]; then
    echo "Error: ELF file not found: $ELF_FILE"
    exit 1
fi

if [ ! -f "$DATA_FILE" ]; then
    echo "Error: Data file not found: $DATA_FILE"
    exit 1
fi

# Check if objcopy is available
if ! command -v objcopy &> /dev/null; then
    echo "Error: objcopy not found. Please install binutils."
    exit 1
fi

echo "Adding .kotbl section to $ELF_FILE from $DATA_FILE..."

# Add the section with appropriate flags:
# - readonly: section is read-only
# - noload: section is not loaded into memory by the loader
# - contents: section has actual content
# - Note: We do NOT use alloc or load since this is not meant to be loaded
objcopy --add-section .kotbl="$DATA_FILE" \
        --set-section-flags .kotbl=readonly,noload,contents \
        "$ELF_FILE"

if [ $? -eq 0 ]; then
    echo "Successfully added .kotbl section"
    echo ""
    echo "Section information:"
    readelf -S "$ELF_FILE" | grep -A 1 "\.kotbl"
else
    echo "Error: Failed to add .kotbl section"
    exit 1
fi
