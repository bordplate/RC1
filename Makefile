# Force use GNU tools
.DEFAULT_GOAL := all
FIND := /usr/bin/find

ifeq ($(OS),Windows_NT)
	OS_ID := Windows
	PATH_SEP := \\
	COPY := copy
	DELETE := del
	DELETEFLAGS := /s /q
else
	OS_ID := $(shell uname 2>/dev/null || echo Unknown)
	PATH_SEP := /
	COPY := cp
	DELETE := rm
	DELETEFLAGS := -rf

	WINE := wine
endif

# Legacy SN/Wine includes can fail to open under concurrent compiler drivers.
# Serialize the driver, while native cross-assembly stays parallel.
EEGCC = python3 tools/run_ee_compiler.py $(WINE) tools/cc/bin/ee-gcc.exe
CROSS = mipsel-linux-gnu
#CROSS = mips64r5900el-ps2-elf

-include userconfig.mk

# Collect sources
SRC_C   = $(shell $(FIND) code -type f -name '*.c')
SRC_S   = $(shell $(FIND) code -type f -name '*.s' | grep -v "_generated/matchings\|_generated/nonmatchings")
SRC_CPP = $(shell $(FIND) code -type f -name '*.cpp')

INCLUDE = -Icode/include -Itools/cc/lib/gcc-lib/ee/2.95.2/include

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/code
TARGET = $(BUILD_DIR)/boot_elf.elf

BASENAME = SCUS_971.99

OBJS = $(SRC_S:code/%.s=$(OBJ_DIR)/%.o) \
       $(SRC_CPP:code/%.cpp=$(OBJ_DIR)/%.o) \
       $(SRC_C:code/%.c=$(OBJ_DIR)/%.o)

PRODG_DIR = tools/cc

# SN handles symbolic memory macros differently from GNU as. The compiler
# driver selects ps2eeas with -snas; GNU's -Wa,-EL/-I options are not accepted.
SN_AS = $(PRODG_DIR)/lib/gcc-lib/ee/2.95.2/ps2eeas.exe
ASSEMBLER ?= snas
ifeq ($(ASSEMBLER),snas)
ASSEMBLER_FLAGS = -snas
else ifeq ($(ASSEMBLER),gnu)
ASSEMBLER_FLAGS = -Wa,-EL -Wa,-Icode/include
else
$(error ASSEMBLER must be snas or gnu)
endif
COMMON_COMPILE_FLAGS = -G8 -O2 -ffast-math -fno-exceptions $(ASSEMBLER_FLAGS)

.PHONY: setup-snas
setup-snas:
	python3 tools/install_sn_assembler.py --dest $(SN_AS)

$(SN_AS):
	python3 tools/install_sn_assembler.py --dest $@

# Verified against the full boot image; do not propagate into 989snd.
$(OBJ_DIR)/game/menu.o $(OBJ_DIR)/game/menu_post_mid.o \
    $(OBJ_DIR)/game/menu_post_gadgets.o: \
    PRIVATE_COMPILE_FLAGS = -fno-schedule-insns
$(OBJ_DIR)/game/transition.o: PRIVATE_COMPILE_FLAGS = -fno-schedule-insns -mno-split-addresses
$(OBJ_DIR)/game/menu_post.o $(OBJ_DIR)/game/menu_post_pages.o \
    $(OBJ_DIR)/game/menu_post_pages_end.o: PRIVATE_COMPILE_FLAGS = -fno-schedule-insns -mno-split-addresses
$(OBJ_DIR)/game/movie/movie_mid.o $(OBJ_DIR)/game/movie/videodec_post.o \
    $(OBJ_DIR)/game/movie/movie_post_audio.o: \
    PRIVATE_COMPILE_FLAGS = -mno-split-addresses
# endDisplay__Fv stores the named in-window movieDisplayActive flag; the flag
# keeps the store a single unsplittable pseudo that ps2eeas expands in place
# to the original's lui at / sw zero pair (a plain named store lets EGC
# schedule the split apart and move the store into the jr delay slot).
$(OBJ_DIR)/game/movie/disp.o: PRIVATE_COMPILE_FLAGS = -mno-split-addresses
$(OBJ_DIR)/game/permcb.o: PRIVATE_COMPILE_FLAGS = -mno-split-addresses
# VU1 chain functions reload the head pointer before every packet store; the
# flag turns the named in-window head global into self-based absolute loads.
$(OBJ_DIR)/game/vuchain.o: PRIVATE_COMPILE_FLAGS = -mno-split-addresses
$(OBJ_DIR)/game/menu_callbacks.o: PRIVATE_COMPILE_FLAGS = -fno-schedule-insns2
$(OBJ_DIR)/game/pause_sched.o: PRIVATE_COMPILE_FLAGS = -fno-schedule-insns
$(OBJ_DIR)/game/pause_post.o: PRIVATE_COMPILE_FLAGS = -G0

# Existing source forms in these TUs rely on GNU macro expansion/scheduling.
# Keep their assembler local until each source migration passes full parity.
# See decomp_state/notes/sn_toolchain_assemblers.md for measured differences.
$(OBJ_DIR)/989snd/ee/989snd.o $(OBJ_DIR)/game/draw.o \
    $(OBJ_DIR)/game/hud.o $(OBJ_DIR)/game/mobyutil.o \
    $(OBJ_DIR)/game/movie/vobuf.o: \
    ASSEMBLER_FLAGS = -Wa,-EL -Wa,-Icode/include

# Isolated experiments, never linked into the game. PROBE_FLAGS are appended.
.PHONY: probe
probe: $(SN_AS)
	@test -n "$(PROBE_SOURCE)" -a -n "$(PROBE_OUT)" || (echo 'Set PROBE_SOURCE and PROBE_OUT (without extension)'; exit 1)
	@mkdir -p $(dir $(PROBE_OUT))
	$(EEGCC) -S $(COMMON_COMPILE_FLAGS) $(PROBE_FLAGS) $(INCLUDE) -B$(PRODG_DIR)/lib/gcc-lib/ee/2.95.2/ $(PROBE_SOURCE) -o $(PROBE_OUT).s
	$(EEGCC) -c $(COMMON_COMPILE_FLAGS) $(PROBE_FLAGS) $(INCLUDE) -B$(PRODG_DIR)/lib/gcc-lib/ee/2.95.2/ $(PROBE_SOURCE) -o $(PROBE_OUT).o

all: $(TARGET)

# Flag/header changes must not silently reuse objects from an older experiment.
$(SRC_CPP:code/%.cpp=$(OBJ_DIR)/%.o) $(SRC_C:code/%.c=$(OBJ_DIR)/%.o): Makefile tools/run_ee_compiler.py $(SN_AS) $(wildcard userconfig.mk code/include/*.h)

.SECONDEXPANSION:

split:
	python -m splat split config/RC1.yaml --disassemble-all

iso:
	$(WRENCHFOLDER)/wrenchbuild pack assets $(WRENCHFOLDER)/underlay -a rac -o build/build.iso -h release --flusher-thread-hack

clean:
	rm -rf build
	rm -rf code/_generated
	rm -rf $(BASENAME).ld
	
$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CROSS)-strip $(OBJS) -N dummy-symbol-name
	# _gp must match the runtime $gp: crt0 loads D_00166C00 into $gp at boot.
	$(CROSS)-ld -EL -m elf32lr5900 --defsym _gp=0x166c00 -T $(BUILD_DIR)/undefined_funcs_auto.txt -T $(BUILD_DIR)/undefined_syms_auto.txt -T config/linker_aliases.ld -T $(BASENAME).ld -Map build/$(BASENAME).ld $(OBJS) -o $@
	$(CROSS)-objcopy $(TARGET) build/boot_elf.elf -O binary

$(OBJ_DIR)/%.o: code/%.s
	@mkdir -p $(dir $@)
	$(CROSS)-as $(INCLUDE) -EL -no-pad-sections -march=5900 -mabi=eabi $< -o $@

$(OBJ_DIR)/%.o: code/%.c $$(wildcard code/_generated/nonmatchings/$$*/*.s code/_generated/matchings/$$*/*.s)
	@mkdir -p $(dir $@)
	$(EEGCC) -c $(COMMON_COMPILE_FLAGS) $(PRIVATE_COMPILE_FLAGS) $(INCLUDE) -B$(PRODG_DIR)/lib/gcc-lib/ee/2.95.2/ $< -o $@

$(OBJ_DIR)/%.o: code/%.cpp $$(wildcard code/_generated/nonmatchings/$$*/*.s code/_generated/matchings/$$*/*.s)
	@mkdir -p $(dir $@)
	$(EEGCC) -v -c -x c++ $(COMMON_COMPILE_FLAGS) $(PRIVATE_COMPILE_FLAGS) $(INCLUDE) -B$(PRODG_DIR)/lib/gcc-lib/ee/2.95.2/ $< -o $@
