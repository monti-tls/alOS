# Tools
CC  = arm-unknown-eabi-gcc
AS  = arm-unknown-eabi-gcc
LD  = arm-unknown-eabi-gcc
CPP = arm-unknown-eabi-cpp
FMT = clang-format

# Directories
SRC_DIR = src
INC_DIR = inc
TMP_DIR = obj
BIN_DIR = bin
IRD_DIR = initrd
MOD_DIR = modules
DOX_DIR = doxygen

# Configuration
PRODUCT = alOS
DEFINES = -DKMALLOC_POOL_SIZE=32768 \
          -DKMALLOC_POOL_DEPTH=10 \
          -DKMALLOC_ALIGNMENT=4
CC_FLAGS =
AS_FLAGS =
LD_FLAGS =

# Mandatory CC flags
CC_FLAGS += -std=c11
CC_FLAGS += -Wall -Wextra
CC_FLAGS += -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork
CC_FLAGS += -mfloat-abi=soft
CC_FLAGS += $(DEFINES)
CC_FLAGS += -I$(INC_DIR) -I$(SRC_DIR)

# Mandatory AS flags
CC_FLAGS += $(DEFINES)
AS_FLAGS += -I$(INC_DIR) -I$(SRC_DIR)

# Mandatory LD flags
LD_FLAGS += -lm -lnosys -nostartfiles

# Format flags
FMT_FLAGS = -i -style=file

# Sources management
C_SUB  = $(shell find $(SRC_DIR) -type d)
H_SUB  = $(shell find $(INC_DIR) -type d)

C_SRC  = $(wildcard $(addsuffix /*.c,$(C_SUB)))
S_SRC  = $(wildcard $(addsuffix /*.s,$(C_SUB)))

C_OBJ  = $(patsubst $(SRC_DIR)/%.c,$(TMP_DIR)/%.o,$(C_SRC))
S_OBJ  = $(patsubst $(SRC_DIR)/%.s,$(TMP_DIR)/%.o,$(S_SRC))

C_DEP  = $(patsubst $(SRC_DIR)/%.c,$(TMP_DIR)/%.d,$(C_SRC))
S_DEP  = $(patsubst $(SRC_DIR)/%.s,$(TMP_DIR)/%.d,$(S_SRC))

C_FMT  = $(foreach d,$(C_SUB),$(patsubst $(d)/%.c,$(d)/fmt-%,$(wildcard $(d)/*.c)))
H_FMT  = $(foreach d,$(H_SUB),$(patsubst $(d)/%.h,$(d)/fmt-%,$(wildcard $(d)/*.h)))

LS_SCR = $(wildcard *.lds)
L_SCR  = $(TMP_DIR)/$(patsubst %.lds,%.ld,$(LS_SCR))

# Product files
KERN_FILE = $(BIN_DIR)/$(PRODUCT).elf
IRD_FILE  = $(TMP_DIR)/$(IRD_DIR).tar
IRD_OBJ   = $(TMP_DIR)/$(IRD_DIR).o

# Top-level
all: kernel

kernel: all_modules $(KERN_FILE)

initrd_img: all_modules $(IRD_FILE)

all_modules: | $(MOD_DIR)
	@$(MAKE) --no-print-directory -C $(MOD_DIR)

.PHONY: doxygen
doxygen:
	@doxygen Doxyfile

.PHONY: clean
clean: modules
	@rm -rf $(BIN_DIR) $(TMP_DIR)
	@$(MAKE) --no-print-directory -C $(MOD_DIR) $@
	@rm -rf $(DOX_DIR)

.PHONY: format
format: $(C_FMT) $(H_FMT)
	@$(MAKE) --no-print-directory -C $(MOD_DIR) $@

# Dependencies
-include $(C_DEPS) $(S_DEPS)

# Translation
$(KERN_FILE): $(IRD_OBJ) $(C_OBJ) $(S_OBJ) $(L_SCR)
	@mkdir -p $(@D)
	@echo "(LD)      $@"
	@$(LD) -o $@ $(filter-out $(L_SCR),$^) $(LD_FLAGS) -T$(L_SCR)

$(IRD_OBJ): $(IRD_FILE)
	@mkdir -p $(@D)
	@echo "(AS)      $@"
	@printf ".section .initrd\n.incbin \"$(IRD_FILE)\"\n" \
	| $(CC) $(AS_FLAGS) -c -o $@ -x assembler -

$(IRD_FILE): $(shell find $(IRD_DIR) -type f)
	@mkdir -p $(@D)
	@echo "(TAR)     $@"
	@tar -cf $(IRD_FILE) $(IRD_DIR)/*

$(L_SCR): $(LS_SCR)
	@mkdir -p $(@D)
	@echo "(CPP)     $@"
	@$(CPP) $(DEFINES) -P -C $< -o $@

$(TMP_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	@echo "(CC)      $<"
	@$(CC) $(CC_FLAGS) -MMD -c $< -o $@

$(TMP_DIR)/%.o: $(SRC_DIR)/%.s
	@mkdir -p $(@D)
	@echo "(AS)      $<"
	@$(AS) $(AS_FLAGS) -MMD -c $< -o $@

# Format
fmt-%: %.c
	@$(FMT) $(FMT_FLAGS) $<
	@echo "(FMT)     $<"

fmt-%: %.h
	@$(FMT) $(FMT_FLAGS) $<
	@echo "(FMT)     $<"
