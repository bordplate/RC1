# RC1 Decomp
Unpack NTSC Ratchet & Clank (SCUS\_971.99) with [Wrench](https://github.com/chaoticgd/wrench). Move the contents of `rac_scus_971_99` from Wrench to the `assets/` folder.  

The process described here is only tested on Linux. It should work fine through WSL.

When building, you should end up with a binary that matches the original game's boot binary byte for byte. There is no consideration for the game's overlays here yet. 

Most of this project is built using the [Sly1 decomp](https://github.com/TheOnlyZac/sly1) as reference. They also have a lot of documentation on the practical stuff around how to decomp functions.

## Setup
You need Python >3.9 to run [splat](https://github.com/ethteck/splat) to configure the thing. Install the requirements:
```sh
# venv is optional, but recommended
python3 -m venv .venv
source .venv/bin/activate

# Install splat
pip install -r requirements.txt

# Split the main game binary into asm
make split  # runs `python -m splat split config/RC1.yaml --disassemble-all`

# Compile
make
```

## Generated files
Splat generates files based on the config in `config/RC1.yaml`. The generated code is taken from the original game and placed under `code/_generated/`.   

`make clean` to clean generated files and build artifacts.

## Compiler installation
We need to use the original (or similar) compiler to the one that was used in the original compilation of the game. This is the SCE/ProDG EEGCC (EmotionEngine GCC).  

These are the required packages:
```sh
wine32  # Not needed on Windows
binutils-mips-linux-gnu
make
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

### Windows
You will need to bring in the `binutils-mips-linux-gnu` tools. You can do this with [MSYS2](https://www.msys2.org) by compiling
the tools yourself. Open the MSYS2 MinGW64 terminal:
```sh
pacman -Syy
pacman -S base-devel git make gcc mpfr-devel gmp-devel texinfo \
  mingw-w64-x86_64-toolchain  # Just choose `all` for the mingw toolchain install
git clone git://sourceware.org/git/binutils-gdb.git
cd binutils-gdb
mkdir build && cd build

../configure \
  --host=x86_64-w64-mingw32 \
  --target=mips-linux-gnu \
  --prefix=/mingw64/mips-linux-gnu \
  --disable-nls \
  --disable-gdb \
  --disable-gdbserver \
  --disable-sim \
  --disable-gprofng \
  --disable-werror

make -j$(nproc)
make install
```

Copy `userconfig.template.mk` to `userconfig.mk` and set this:
```makefile
export PATH := C:/msys64/mingw64/mips-linux-gnu/bin:$(PATH)
CROSS = mips-linux-gnu
```

For the tooling to work properly on Windows for me, I had to install the Python `requirements.txt` with the `--user` flag:
```sh
python -m pip install -r requirements.txt --user
```

## Decompiling
Use [decomp.me](https://decomp.me/) with the EE GCC 2.95.2 (SN BUILD v2.74) compiler to try to match your decomp with the original assembly for the relevant function. You can generate the necessary context with the following command:
```sh
python tools/m2ctx/m2ctx.py <file containing function you're decompiling>
```
This will generate a `ctx.c` file the current directory that you can use as context in decomp.me.  

I'm not sure it's the exact same compiler Insomniac used for RC1, and I have done no research on it. It's the same compiler used for the Sly1 decomp, and Sly1 came out around the same time as RC1.

### Comparing compiled binary
The compiles binary should match the original game's binary byte for byte. You can compare them with checksums or something like:
```sh
cmp build/boot_elf.elf assets/boot_elf.elf
```
