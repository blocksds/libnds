# SPDX-License-Identifier: Zlib
#
# Copyright (c) 2023 Antonio Niño Díaz

# Defines
# -------

LIBNDS_MAJOR	:= 1
LIBNDS_MINOR	:= 8
LIBNDS_PATCH	:= 0
VERSION		:= $(LIBNDS_MAJOR).$(LIBNDS_MINOR).$(LIBNDS_PATCH)
VERSION_HEADER	:= include/nds/libversion.h

# Tools
# -----

MAKE		:= make
RM		:= rm -rf

# Targets
# -------

.PHONY: all arm7 arm9 clean docs

all: $(VERSION_HEADER) arm9 arm7

$(VERSION_HEADER): Makefile
	@echo "#ifndef __LIBNDSVERSION_H__" > $@
	@echo "#define __LIBNDSVERSION_H__" >> $@
	@echo >> $@
	@echo "#define _LIBNDS_MAJOR_ $(LIBNDS_MAJOR)" >> $@
	@echo "#define _LIBNDS_MINOR_ $(LIBNDS_MINOR)" >> $@
	@echo "#define _LIBNDS_PATCH_ $(LIBNDS_PATCH)" >> $@
	@echo >> $@
	@echo '#define _LIBNDS_STRING "libNDS Release '$(LIBNDS_MAJOR).$(LIBNDS_MINOR).$(LIBNDS_PATCH)'"' >> $@
	@echo >> $@
	@echo "#endif // __LIBNDSVERSION_H__" >> $@

arm9:
	@+$(MAKE) -f Makefile.arm9 --no-print-directory

arm7:
	@+$(MAKE) -f Makefile.arm7 --no-print-directory

clean:
	@echo "  CLEAN"
	@$(RM) $(VERSION_HEADER) lib build

docs:
	@echo "  DOXYGEN"
	@doxygen Doxyfile
