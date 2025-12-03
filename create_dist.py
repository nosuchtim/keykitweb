#!/usr/bin/env python3
"""
Create a stripped-down distribution zip file containing only the files
needed to run KeyKit in the browser.

Run this script from the main keykitwasm repository directory.

Usage:
    python create_dist.py <output_zipfile>

Example:
    python create_dist.py keykit_dist.zip
"""

import os
import sys
import json
import zipfile
import subprocess


def run_build_steps(repo_path):
    """Run manifest generation and WASM build before creating the distribution."""

    print("=" * 50)
    print("Running build steps...")
    print("=" * 50)

    # Run lib manifest generator
    lib_dir = os.path.join(repo_path, "lib")
    lib_manifest_script = os.path.join(lib_dir, "generate_manifest.py")
    if os.path.exists(lib_manifest_script):
        print("\n[1/2] Generating lib manifest...")
        result = subprocess.run(
            [sys.executable, "generate_manifest.py"],
            cwd=lib_dir,
            capture_output=True,
            text=True
        )
        if result.returncode != 0:
            print(f"Error running lib manifest generator:")
            print(result.stderr)
            sys.exit(1)
        print(result.stdout)
        print("Library manifest generated.")
    else:
        print(f"Warning: lib manifest script not found: {lib_manifest_script}")

    # Run WASM build
    build_script = os.path.join(repo_path, "build_wasm.py")
    if os.path.exists(build_script):
        print("\n[2/2] Building WASM...")
        result = subprocess.run(
            [sys.executable, "build_wasm.py"],
            cwd=repo_path,
            capture_output=True,
            text=True
        )
        if result.returncode != 0:
            print(f"Error running WASM build:")
            print(result.stderr)
            sys.exit(1)
        print(result.stdout)
        print("WASM build completed.")
    else:
        print(f"Error: build script not found: {build_script}")
        sys.exit(1)

    print("\n" + "=" * 50)
    print("Build steps completed successfully!")
    print("=" * 50 + "\n")


def create_dist(repo_path, zip_path):
    """Create a distribution zip file with only runtime-required files."""

    # Validate repo path
    if not os.path.isdir(repo_path):
        print(f"Error: Repository path does not exist: {repo_path}")
        sys.exit(1)

    # Run build steps first
    run_build_steps(repo_path)

    # Verify required files exist after build
    required_checks = [
        os.path.join(repo_path, "keykit.html"),
        os.path.join(repo_path, "keykit.js"),
        os.path.join(repo_path, "keykit.wasm"),
        os.path.join(repo_path, "lib", "lib_manifest.json"),
    ]

    missing = [f for f in required_checks if not os.path.exists(f)]
    if missing:
        print("Error: Build completed but required files are missing:")
        for f in missing:
            print(f"  Missing: {f}")
        sys.exit(1)

    # Ensure zip_path ends with .zip
    if not zip_path.endswith('.zip'):
        zip_path += '.zip'

    # Get the subdirectory name from the zip filename (without .zip extension)
    zip_basename = os.path.basename(zip_path)
    subdir = zip_basename[:-4]  # Remove .zip extension

    # Remove existing zip file if present
    if os.path.exists(zip_path):
        print(f"Warning: Zip file exists, removing: {zip_path}")
        os.remove(zip_path)

    print(f"Creating distribution zip: {zip_path}")
    print(f"Files will be in subdirectory: {subdir}/")

    with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zf:
        # Files from main repo directory (the built WASM application)
        app_files = [
            "keykit.html",
            "keykit.js",
            "keykit.wasm",
            "keykit.ico",
        ]

        for filename in app_files:
            src_file = os.path.join(repo_path, filename)
            if os.path.exists(src_file):
                zf.write(src_file, f"{subdir}/{filename}")
                print(f"Added: {subdir}/{filename}")
            else:
                print(f"Warning: File not found: {src_file}")

        # Add lib/ directory (KeyKit library files)
        lib_src = os.path.join(repo_path, "lib")

        if os.path.isdir(lib_src):
            # Read manifest to get only needed files
            manifest_path = os.path.join(lib_src, "lib_manifest.json")
            with open(manifest_path, 'r') as f:
                manifest_files = json.load(f)

            # Add manifest
            zf.write(manifest_path, f"{subdir}/lib/lib_manifest.json")
            print(f"Added: {subdir}/lib/lib_manifest.json")

            # Add all files listed in manifest
            added_count = 0
            for filename in manifest_files:
                src_file = os.path.join(lib_src, filename)
                if os.path.exists(src_file):
                    zf.write(src_file, f"{subdir}/lib/{filename}")
                    added_count += 1
                else:
                    print(f"Warning: Manifest file not found: {src_file}")

            print(f"Added: {added_count} library files to {subdir}/lib/")
        else:
            print(f"Warning: lib directory not found: {lib_src}")

        # Add music/ directory (sample MIDI files)
        music_src = os.path.join(repo_path, "music")

        if os.path.isdir(music_src):
            music_count = 0
            mid_files = []
            for filename in os.listdir(music_src):
                if filename.endswith('.mid'):
                    src_file = os.path.join(music_src, filename)
                    zf.write(src_file, f"{subdir}/music/{filename}")
                    mid_files.append(filename)
                    music_count += 1

            # Create and add manifest for music directory
            music_manifest = json.dumps(sorted(mid_files), indent=2)
            zf.writestr(f"{subdir}/music/music_manifest.json", music_manifest)
            print(f"Added: {music_count} MIDI files to {subdir}/music/")
            print(f"Added: {subdir}/music/music_manifest.json")

        # Add local/ directory structure (for user files)
        local_src = os.path.join(repo_path, "local")

        if os.path.isdir(local_src):
            for root, dirs, files in os.walk(local_src):
                for filename in files:
                    src_file = os.path.join(root, filename)
                    rel_path = os.path.relpath(src_file, repo_path)
                    zf.write(src_file, f"{subdir}/{rel_path}")
            print(f"Added: {subdir}/local/ directory")
        else:
            # Create empty local directory structure with placeholder files
            # (zip files can't store empty directories, so we add .gitkeep files)
            zf.writestr(f"{subdir}/local/music/.gitkeep", "")
            zf.writestr(f"{subdir}/local/pages/.gitkeep", "")
            print(f"Added: empty {subdir}/local/ directory structure")

        # Add serve.py for convenience
        serve_src = os.path.join(repo_path, "serve.py")
        if os.path.exists(serve_src):
            zf.write(serve_src, f"{subdir}/serve.py")
            print(f"Added: {subdir}/serve.py")

    # Get file size
    zip_size = os.path.getsize(zip_path)
    zip_size_mb = zip_size / (1024 * 1024)

    # Summary
    print("\n" + "=" * 50)
    print("Distribution zip created successfully!")
    print("=" * 50)
    print(f"\nOutput file: {zip_path}")
    print(f"Size: {zip_size_mb:.2f} MB")
    print(f"Contents in: {subdir}/")
    print("\nTo use:")
    print("  1. Extract the zip file")
    print(f"  2. python {subdir}/serve.py")
    print(f"  3. Open http://localhost:8000/{subdir}/keykit.html")


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(1)

    # Use current directory as repo path
    repo_path = os.getcwd()

    # Put output in dist/ directory
    dist_dir = os.path.join(repo_path, "dist")
    os.makedirs(dist_dir, exist_ok=True)

    zip_path = os.path.join(dist_dir, sys.argv[1])

    # Verify we're in the right directory
    if not os.path.exists(os.path.join(repo_path, "build_wasm.py")):
        print("Error: This script must be run from the keykitwasm repository directory.")
        print(f"Current directory: {repo_path}")
        print("Expected to find: build_wasm.py")
        sys.exit(1)

    create_dist(repo_path, zip_path)


if __name__ == "__main__":
    main()
