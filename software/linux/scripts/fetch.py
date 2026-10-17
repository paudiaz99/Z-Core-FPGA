#!/usr/bin/env python3
"""Fetch SHA256-pinned upstream sources; never operates on hardware."""
import hashlib
from pathlib import Path
import tarfile
import urllib.request
ROOT = Path(__file__).resolve().parents[1]
SOURCES = (
    ("linux-6.12.10", "https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.12.10.tar.xz",
     "4a516e5ed748537a73cb42ec47fbbeb6df8b1298e8892c29c0e91de79095b297"),
    ("buildroot-2025.02.1", "https://buildroot.org/downloads/buildroot-2025.02.1.tar.xz",
     "a3216907e7ce1bbd38413e20b0861641b160ecc40dd7b598febb7f12984b3206"),
)
def main():
    dest = ROOT / "sources"
    dest.mkdir(exist_ok=True)
    for name, url, digest in SOURCES:
        archive = dest / (name + ".tar.xz")
        if not archive.exists():
            temp = archive.with_suffix(".part")
            print("Downloading", url, flush=True)
            urllib.request.urlretrieve(url, temp)
            temp.rename(archive)
        if hashlib.sha256(archive.read_bytes()).hexdigest() != digest:
            raise SystemExit(f"SHA256 mismatch: {archive}; remove and retry")
        if not (dest / name).exists():
            with tarfile.open(archive) as tar:
                tar.extractall(dest, filter="data")
        print(name, "verified")
if __name__ == "__main__":
    main()
