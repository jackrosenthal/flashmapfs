OUTDIR ?= build
CC ?= cc
_create_outdir := $(shell [ -d $(OUTDIR) ] || mkdir -p $(OUTDIR))

# Recursive wildcard function, stackoverflow.com/questions/2483182
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

DOTCONFIG := $(OUTDIR)/.config
AUTOHEADER := $(OUTDIR)/kconfig.h
CONFIG_OUT := $(OUTDIR)/build.conf

_ := $(shell KCONFIG_CONFIG=$(DOTCONFIG) \
	     3rdparty/kconfiglib/genconfig.py \
		--header-path="$(AUTOHEADER)" \
		--config-out="$(CONFIG_OUT)")

include $(CONFIG_OUT)

cflags-y := -std=gnu17 -Wall -Wstrict-prototypes -Wmissing-prototypes \
	-Wundef -Wmissing-declarations -Wno-address-of-packed-member \
	-Iinclude -include "$(AUTOHEADER)" \
	-DFUSE_USE_VERSION=35 \
	$(CONFIG_COMPILER_DEBUG_SYMBOLS_FLAG) \
	$(CONFIG_COMPILER_OPT_FLAG) \
	$(CONFIG_COMPILER_ASAN_FLAG) \
	$(CONFIG_COMPILER_PROFILE_FLAG) \
	$(CONFIG_COMPILER_COVERAGE_FLAG) \
	$(CONFIG_COMPILER_WARNINGS_ARE_ERRORS_FLAG) \
	$(CONFIG_LTO_FLAG)

ldflags-y := $(CONFIG_LINK_STATIC_FLAG) \
	$(CONFIG_LTO_FLAG) \
	$(CONFIG_COMPILER_DEBUG_SYMBOLS_FLAG) \
	$(CONFIG_COMPILER_ASAN_FLAG) \
	$(CONFIG_COMPILER_PROFILE_FLAG) \
	$(CONFIG_COMPILER_COVERAGE_FLAG)

pkg-config-y := pkg-config
pkg-config-$(CONFIG_LINK_STATIC) += --static

libs-y := fuse3
libs-$(CONFIG_FLASHMAP_DYNLIB) += fmap

cflags-y += $(foreach lib,$(libs-y),$(shell \
	if $(pkg-config-y) --exists "$(lib)"; then \
		$(pkg-config-y) --cflags "$(lib)"; \
	fi))
ldflags-y += $(foreach lib,$(libs-y),$(shell \
	if $(pkg-config-y) --exists "$(lib)"; then \
		$(pkg-config-y) --libs "$(lib)"; \
	else \
		echo "-l$(lib)"; \
	fi))

cflags-$(CONFIG_FLASHMAP_INTERNAL) += -I3rdparty/flashmap

srcs-y := arena.c boolean_flag_file.c fs.c main.c mmap_file.c route.c \
	raw_file.c str_file.c version_file.c
srcs-$(CONFIG_GBB) += gbb.c
srcs-$(CONFIG_FLASHMAP_INTERNAL) += 3rdparty/flashmap/fmap.c

objfiles := $(patsubst %.c,$(OUTDIR)/%.o,$(srcs-y))
dirs := $(sort $(dir $(objfiles)))
_create_dirs := $(foreach d,$(dirs),$(shell [ -d $(d) ] || mkdir -p $(d)))

target := fmapfs

ifeq ($(V),)
cmd = @printf '  %-6s %s\n' $(cmd_$(1)_name) "$(if $(2),$(2),"$@")" ; $(call cmd_$(1),$(2))
else
ifeq ($(V),1)
cmd = $(call cmd_$(1),$(2))
else
cmd = @$(call cmd_$(1),$(2))
endif
endif

DEPFLAGS = -MMD -MP -MF $@.d

cmd_c_to_o_name = CC
cmd_c_to_o = $(CC) $(cflags-y) $(DEPFLAGS) -c $< -o $@

cmd_o_to_elf_name = LD
cmd_o_to_elf = $(CC) $(ldflags-y) $^ -o $@

cmd_clean_name = CLEAN
cmd_clean = rm -rf $(1)

cmd_conf_name = CONF
cmd_conf = KCONFIG_CONFIG=$(DOTCONFIG) 3rdparty/kconfiglib/$(1).py

.SECONDARY:
.PHONY: all
all: $(OUTDIR)/$(target)

-include $(call rwildcard,$(OUTDIR),*.d)

CONFIG_UIS := menuconfig guiconfig
.PHONY: $(CONFIG_UIS)
$(CONFIG_UIS):
	$(call cmd,conf,$@)

$(OUTDIR)/$(target): $(objfiles)
	$(call cmd,o_to_elf)

# Depending on CONFIG_OUT will force a complete rebuild when config
# changes.
$(OUTDIR)/%.o: %.c $(CONFIG_OUT)
	$(call cmd,c_to_o)

.PHONY: clean
clean:
	$(call cmd,clean,$(OUTDIR))
