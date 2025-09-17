EEGCC = wine tools/cc/bin/ee-gcc.exe
CROSS = mipsel-linux-gnu
#CROSS = mips64r5900el-ps2-elf

SRC_C   = $(shell find code -type f -name '*.c')
SRC_S   = $(shell find code -type f -name '*.s' | grep -v _generated/nonmatchings)
SRC_CPP = $(shell find code -type f -name '*.cpp')

INCLUDE = code/include

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/code
TARGET = $(BUILD_DIR)/boot_elf.elf

BASENAME = SCUS_971.99

OBJS = $(SRC_S:code/%.s=$(OBJ_DIR)/%.o) \
       $(SRC_CPP:code/%.cpp=$(OBJ_DIR)/%.o) \
       $(SRC_C:code/%.c=$(OBJ_DIR)/%.o)

PRODG_DIR = tools/cc

COMMON_COMPILE_FLAGS = -G0

all: $(TARGET)

split:
	python -m splat split config/RC1.yaml --disassemble-all

clean:
	rm -rf build
	rm -rf code/_generated
	rm -rf $(BASENAME).ld
	
$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CROSS)-strip $(OBJS) -N dummy-symbol-name
	$(CROSS)-ld -EL -T $(BUILD_DIR)/undefined_funcs_auto.txt -T $(BUILD_DIR)/undefined_syms_auto.txt -T $(BASENAME).ld -Map build/$(BASENAME).ld $(OBJS) -o $@
	$(CROSS)-objcopy $(TARGET) build/boot_elf.elf -O binary

$(OBJ_DIR)/%.o: code/%.s
	@mkdir -p $(dir $@)
	$(CROSS)-as -I$(INCLUDE) -EL -no-pad-sections -march=5900 -mabi=eabi $< -o $@

$(OBJ_DIR)/%.o: code/%.c
	@mkdir -p $(dir $@)
	$(EEGCC) -c $(COMMON_COMPILE_FLAGS) -B$(PRODG_DIR)/lib/gcc-lib/ee/2.95.2/ $< -o $@

$(OBJ_DIR)/%.o: code/%.cpp
	@mkdir -p $(dir $@)
	$(EEGCC) -c -x c++ $(COMMON_COMPILE_FLAGS) -I$(INCLUDE) -B$(PRODG_DIR)/lib/gcc-lib/ee/2.95.2/ $< -o $@

