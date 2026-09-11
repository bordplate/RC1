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

EEGCC = $(WINE) tools/cc/bin/ee-gcc.exe
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

COMMON_COMPILE_FLAGS = -G8 -O2 -ffast-math -fno-exceptions -Wa,-EL -Wa,-Icode/include

# Verified against the full boot image; do not propagate into 989snd.
$(OBJ_DIR)/game/menu.o $(OBJ_DIR)/game/menu_post_mid.o \
    $(OBJ_DIR)/game/menu_post_gadgets.o $(OBJ_DIR)/game/transition.o: \
    PRIVATE_COMPILE_FLAGS = -fno-schedule-insns
$(OBJ_DIR)/game/menu_post.o $(OBJ_DIR)/game/menu_post_pages.o \
    $(OBJ_DIR)/game/menu_post_pages_end.o: PRIVATE_COMPILE_FLAGS = -fno-schedule-insns -mno-split-addresses
$(OBJ_DIR)/game/menu_callbacks.o: PRIVATE_COMPILE_FLAGS = -fno-schedule-insns2
$(OBJ_DIR)/game/pause_sched.o: PRIVATE_COMPILE_FLAGS = -fno-schedule-insns

# Isolated experiments, never linked into the game. PROBE_FLAGS are appended.
.PHONY: probe
probe:
	@test -n "$(PROBE_SOURCE)" -a -n "$(PROBE_OUT)" || (echo 'Set PROBE_SOURCE and PROBE_OUT (without extension)'; exit 1)
	@mkdir -p $(dir $(PROBE_OUT))
	$(EEGCC) -S $(COMMON_COMPILE_FLAGS) $(PROBE_FLAGS) $(INCLUDE) -B$(PRODG_DIR)/lib/gcc-lib/ee/2.95.2/ $(PROBE_SOURCE) -o $(PROBE_OUT).s
	$(EEGCC) -c $(COMMON_COMPILE_FLAGS) $(PROBE_FLAGS) $(INCLUDE) -B$(PRODG_DIR)/lib/gcc-lib/ee/2.95.2/ $(PROBE_SOURCE) -o $(PROBE_OUT).o

all: $(TARGET)

# Flag/header changes must not silently reuse objects from an older experiment.
$(SRC_CPP:code/%.cpp=$(OBJ_DIR)/%.o) $(SRC_C:code/%.c=$(OBJ_DIR)/%.o): Makefile $(wildcard userconfig.mk code/include/*.h)

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
