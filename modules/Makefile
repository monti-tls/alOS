# Enumerate all available modules, ignore .d directories
MODULES = $(shell find . -maxdepth 1 ! -name '*.d' -type d ! -path . -printf '%P ')
TARGETS = $(addprefix mod-,$(MODULES))

# Top-level rules
all clean format: modules

modules: $(TARGETS)

mod-%:
	@$(MAKE) --no-print-directory -C $* $(MAKECMDGOALS)
