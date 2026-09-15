#!/usr/bin/env python3
"""Install the verified SN assembler without replacing the compiler or headers."""

import argparse
import hashlib
import io
from pathlib import Path
import tarfile
import urllib.request

URL = ('https://github.com/decompme/compilers/releases/download/compilers/'
       'ee-gcc2.95.2-273a.tar.gz')
ARCHIVE_SHA256 = 'ee9d9a7fccb59aebfa78a5587f6f8059660b91f705acddbc292ad2243c8e562e'
ASSEMBLER_SHA256 = 'c839dd63facabe7b76573c114056be61eaa7b3aa2329dd546930ab8f98898c76'
MEMBER = 'lib/gcc-lib/ee/2.95.2/ps2eeas.exe'


def verify(data, expected, description):
    actual = hashlib.sha256(data).hexdigest()
    if actual != expected:
        raise SystemExit(f'{description}: SHA-256 mismatch: {actual}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dest', type=Path, default=Path('tools/cc') / MEMBER)
    parser.add_argument('--archive', type=Path, help='Use a previously downloaded archive')
    args = parser.parse_args()
    if args.dest.exists():
        verify(args.dest.read_bytes(), ASSEMBLER_SHA256, str(args.dest))
        print(f'Verified SN assembler 1.8.19.316: {args.dest}')
        return
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
    print(f'Installed SN assembler 1.8.19.316: {args.dest}')


if __name__ == '__main__':
    main()
