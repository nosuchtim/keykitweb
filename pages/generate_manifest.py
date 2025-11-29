#!/usr/bin/env python3
"""
Generate a manifest of all page files for runtime loading.
"""

import os
import json
import glob

# Change to the script's directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# File extensions to include
extensions = ['*.kp', '*.KP']

# Collect all matching files
files = []
for pattern in extensions:
    files.extend(glob.glob(pattern))

# Remove duplicates and sort
files = sorted(set(files))

# Write JSON manifest
with open('pages_manifest.json', 'w') as f:
    json.dump(files, f, indent=2)

print(f"Generated pages_manifest.json with {len(files)} page files")
