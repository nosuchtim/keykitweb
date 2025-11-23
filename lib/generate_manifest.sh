#!/bin/bash
# Generate a manifest of all library files for runtime loading

cd "$(dirname "$0")"

# Output to lib_manifest.json
echo "[" > lib_manifest.json

first=true
shopt -s nullglob
for file in *.k *.kc *.kb *.kbm *.exp *.txt *.ppm *.lst; do
    if [ -f "$file" ]; then
        if [ "$first" = true ]; then
            first=false
        else
            echo "," >> lib_manifest.json
        fi
        echo -n "  \"$file\"" >> lib_manifest.json
    fi
done

echo "" >> lib_manifest.json
echo "]" >> lib_manifest.json

echo "Generated lib_manifest.json with library files"
