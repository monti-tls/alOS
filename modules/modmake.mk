# Tools
CC      = arm-unknown-eabi-gcc
LD      = arm-unknown-eabi-ld
FMT     = clang-format

# Directories
SRC_DIR     = src
INC_DIR     = inc
TMP_DIR     = obj
BIN_DIR     = bin
KERNEL_ROOT = ../../
DIST_PATH   = $(KERNEL_ROOT)/initrd/modules/

# Module-specific
include module.mk

# Mandatory CC flags
CC_FLAGS += -std=c11 -fno-common
CC_FLAGS += -mlong-calls -mword-relocations
CC_FLAGS += $(DEFINES) -I$(INC_DIR)

# Format flags
FMT_FLAGS = -i -style=file

# Sources management
C_SUB = $(shell find $(SRC_DIR) -type d)
H_SUB = $(shell find $(INC_DIR) -type d)

C_SRC = $(wildcard $(addsuffix /*.c,$(C_SUB)))
S_SRC = $(wildcard $(addsuffix /*.s,$(C_SUB)))

C_OBJ = $(patsubst $(SRC_DIR)/%.c,$(TMP_DIR)/%.o,$(C_SRC))
S_OBJ = $(patsubst $(SRC_DIR)/%.s,$(TMP_DIR)/%.o,$(S_SRC))

C_DEP = $(patsubst $(SRC_DIR)/%.c,$(TMP_DIR)/%.d,$(C_SRC))
S_DEP = $(patsubst $(SRC_DIR)/%.s,$(TMP_DIR)/%.d,$(S_SRC))

C_FMT = $(foreach d,$(C_SUB),$(patsubst $(d)/%.c,$(d)/fmt-%,$(wildcard $(d)/*.c)))
H_FMT = $(foreach d,$(H_SUB),$(patsubst $(d)/%.h,$(d)/fmt-%,$(wildcard $(d)/*.h)))

# Products
MOD_FILE  = $(BIN_DIR)/$(MOD_NAME).ko
DIST_FILE = $(DIST_PATH)/$(MOD_NAME).ko

# Top-level
all: modpost

modpost: $(DIST_FILE)

.PHONY: clean
clean:
	@rm -rf $(TMP_DIR) $(BIN_DIR) $(DIST_FILE)

.PHONY: format
format: $(C_FMT) $(H_FMT)

# Dependencies
-include $(C_DEP) $(S_DEP)

# Translation
$(DIST_FILE): $(MOD_FILE)
	@echo "(MODPOST) $<"
	@mkdir -p $(DIST_PATH)
	@cp $< $@

$(MOD_FILE): $(C_OBJ) $(S_OBJ)
	@mkdir -p $(@D)
	@echo "(LD)      $@"
	@$(LD) -r -o $@ $^ $(LD_FLAGS)

$(TMP_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	@echo "(CC)      $<"
	@$(CC) -o $@ -MMD $(CC_FLAGS) -c $<

$(TMP_DIR)/%.o: $(SRC_DIR)/%.s
	@mkdir -p $(@D)
	@echo "(AS)      $<"
	@$(CC) -o $@ -MMD $(CC_FLAGS) -c $<

# Format
fmt-%: %.c
	@$(FMT) $(FMT_FLAGS) $<
	@echo "(FMT)     $<"

fmt-%: %.h
	@$(FMT) $(FMT_FLAGS) $<
	@echo "(FMT)     $<"

# Trace
.PHONY: _trace
_trace:
	@echo $(C_SUB)
	@echo $(H_SUB)
	@echo $(C_SRC)
	@echo $(C_OBJ)
	@echo $(C_DEP)
	@echo $(S_SRC)
	@echo $(S_OBJ)
	@echo $(S_DEP)
	@echo $(C_FMT)
	@echo $(H_FMT)
