#!/usr/bin/env python3

import os
import shutil
import sys

from pathlib import Path

import splat
import splat.scripts.split as split

ROOT = Path(__file__).parent.resolve()

BASENAME = "SCUS_971.99"
CONFIG_FILE ="config/RC1.yaml"


def main():
    split.main([CONFIG_FILE], modes="all", verbose=True)


if __name__ == "__main__":
    main()
