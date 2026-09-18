#!/usr/bin/env python3
"""Install the verified SN assembler without replacing the compiler or headers."""

import argparse
import hashlib
import io
from pathlib import Path
import tarfile
import urllib.request

URL = ('https://github.com/decompme/compilers/releases/download/compilers/'
       'ee-gcc2.95.3-114.tar.gz')
ARCHIVE_SHA256 = 'dbc2c8c764631788d4cbb4c848c3cb0002fded0f4a95bae39e6d8b794391a6cb'
ASSEMBLER_SHA256 = '44bcd9aaa229d8a453730142792d542e56da14761e9677cffcd1a183f603836d'
LEGACY_ASSEMBLER_SHA256 = 'c839dd63facabe7b76573c114056be61eaa7b3aa2329dd546930ab8f98898c76'
MEMBER = 'lib/gcc-lib/ee/2.95.3/ps2eeas.exe'
DEFAULT_DEST = Path('tools/cc/lib/gcc-lib/ee/2.95.2/ps2eeas.exe')


def verify(data, expected, description):
    actual = hashlib.sha256(data).hexdigest()
    if actual != expected:
        raise SystemExit(f'{description}: SHA-256 mismatch: {actual}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dest', type=Path, default=DEFAULT_DEST)
    parser.add_argument('--archive', type=Path, help='Use a previously downloaded archive')
    args = parser.parse_args()
    if args.dest.exists():
        current = hashlib.sha256(args.dest.read_bytes()).hexdigest()
        if current == ASSEMBLER_SHA256:
            print(f'Verified SN assembler 1.9.6.516: {args.dest}')
            return
        if current != LEGACY_ASSEMBLER_SHA256:
            raise SystemExit(f'{args.dest}: unrecognized assembler SHA-256: {current}')
        print(f'Upgrading SN assembler 1.8.19.316: {args.dest}')
    if args.archive:
        archive = args.archive.read_bytes()
    else:
        print(f'Downloading {URL}')
        with urllib.request.urlopen(URL, timeout=60) as response:
            archive = response.read()
    verify(archive, ARCHIVE_SHA256, 'Compiler archive')
    with tarfile.open(fileobj=io.BytesIO(archive), mode='r:gz') as tar:
        with tar.extractfile(MEMBER) as stream:
            assembler = stream.read()
    verify(assembler, ASSEMBLER_SHA256, MEMBER)
    args.dest.parent.mkdir(parents=True, exist_ok=True)
    args.dest.write_bytes(assembler)
    print(f'Installed SN assembler 1.9.6.516: {args.dest}')


if __name__ == '__main__':
    main()
