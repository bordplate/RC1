# RC1 Decomp
Unpack NTSC Ratchet & Clank (SCUS\_971.99) with [Wrench](https://github.com/chaoticgc/wrench). Move the contents of `rac_scus_971_99` from Wrench to the `assets\` folder.  

The process described here is only tested on Linux. It should work fine through WSL.

## Setup
You need Python >3.9 to run splat to configure the thing. Install the requirements:
```
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

