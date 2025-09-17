# RC1 Decomp
Unpack NTSC Ratchet & Clank (SCUS\_971.99) with [Wrench](https://github.com/chaoticgc/wrench). Move the contents of `rac_scus_971_99` from Wrench to the `assets/` folder.  

The process described here is only tested on Linux. It should work fine through WSL.

## Setup
You need Python >3.9 to run splat to configure the thing. Install the requirements:
```sh
# venv is optional, but recommended
python3 -m venv .venv
source .venv/bin/activate

# Install splat
pip install -r requirements.txt

# Split the main game binary into asm
python -m splat split config/RC1.yaml --disassemble-all
```

## Generated files
Splat generates files based on the config in `config/RC1.yaml`. The generated code is taken from the original game and placed under `code/_generated/`.   

`./clean.sh` to clean generated files. 

## Compiler installation
We need to use the original (or similar) compiler to the one that was used in the original compilation of the game. This is the SCE/ProDG EEGCC (EmotionEngine GCC).  

These are the required packages:
```
wine32
binutils-mips-linux-gnu
```

Download the SCE compilers to `tools/`:
```sh
curl -o tools/ee-gcc2.95.2-SN-v2.73a.tar.gz https://bordplate.no/ee-gcc2.95.2-SN-v2.73a.tar.gz
tar -xzf tools/ee-gcc2.95.2-SN-v2.73a.tar.gz -C tools/
```

There are multiple variations of the MIPS compiler that can work, you might have to change the `CROSS` variable in the `Makefile` from `mipsel-linux-gnu` to e.g. `mips-linux-gnu`.

### Debian (Ubuntu, etc.)
On Debian based distros like Ubuntu, you install the 32-bit version of Wine like this:
```sh
dpkg --add-architecture i386
apt-get update
apt-get install wine32
```

Get the MIPS binutils tools:
```sh
apt-get install binutils-mips-linux-gnu
```

### Arch 
Get these packages from AUR with your favorite AUR-thing like `yay`:
```
wine32
mipsel-linux-gnu-binutils
```
