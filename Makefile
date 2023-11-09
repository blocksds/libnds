# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2023

WONDERFUL_TOOLCHAIN	?= /opt/wonderful
LLVM_TEAK_PATH		?= $(WONDERFUL_TOOLCHAIN)/toolchain/llvm-teak/bin/

# Tools
# -----

CP		:= cp
INSTALL		:= install
MAKE		:= make
RM		:= rm -rf

# Verbose flag
# ------------

ifeq ($(VERBOSE),1)
V		:=
else
V		:= @
endif

# Targets
# -------

.PHONY: all arm7 arm9 teak clean docs install

all: arm9 arm7 teak

arm9:
	@+$(MAKE) -f Makefile.arm9 --no-print-directory
	@+$(MAKE) -f Makefile.arm9 --no-print-directory DEBUG=1

arm7:
	@+$(MAKE) -f Makefile.arm7 --no-print-directory
	@+$(MAKE) -f Makefile.arm7 --no-print-directory DEBUG=1

LLVM_TEAK_CLANG_VERSION	:= $(shell $(LLVM_TEAK_PATH)clang --version 2> /dev/null)

teak:
ifeq ($(strip $(LLVM_TEAK_CLANG_VERSION)),)
	@echo "Skipping libteak: toolchain-llvm-teak-llvm not found"
else
	@+$(MAKE) -f Makefile.teak --no-print-directory
	@+$(MAKE) -f Makefile.teak --no-print-directory DEBUG=1
endif

clean:
	@echo "  CLEAN"
	@$(RM) lib build

docs:
	@echo "  DOXYGEN"
	@doxygen Doxyfile

INSTALLDIR	?= /opt/blocksds/core/libs/libnds
INSTALLDIR_ABS	:= $(abspath $(INSTALLDIR))

install: all
	@echo "  INSTALL $(INSTALLDIR_ABS)"
	@test $(INSTALLDIR_ABS)
	$(V)$(RM) $(INSTALLDIR_ABS)
	$(V)$(INSTALL) -d $(INSTALLDIR_ABS)
	$(V)$(CP) -r include lib licenses $(INSTALLDIR_ABS)
