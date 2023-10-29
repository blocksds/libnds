# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2023

# Defines
# -------

LIBNDS_MAJOR	:= 1
LIBNDS_MINOR	:= 8
LIBNDS_PATCH	:= 0
VERSION		:= $(LIBNDS_MAJOR).$(LIBNDS_MINOR).$(LIBNDS_PATCH)
VERSION_HEADER	:= include/nds/libversion.h

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

all: $(VERSION_HEADER) arm9 arm7 teak

$(VERSION_HEADER): Makefile
	@echo "#ifndef LIBNDS_NDS_LIBVERSION_H__" > $@
	@echo "#define LIBNDS_NDS_LIBVERSION_H__" >> $@
	@echo >> $@
	@echo "#define _LIBNDS_MAJOR_ $(LIBNDS_MAJOR)" >> $@
	@echo "#define _LIBNDS_MINOR_ $(LIBNDS_MINOR)" >> $@
	@echo "#define _LIBNDS_PATCH_ $(LIBNDS_PATCH)" >> $@
	@echo >> $@
	@echo '#define _LIBNDS_STRING "libnds release '$(LIBNDS_MAJOR).$(LIBNDS_MINOR).$(LIBNDS_PATCH)'"' >> $@
	@echo >> $@
	@echo "#endif // LIBNDS_NDS_LIBVERSION_H__" >> $@

arm9:
	@+$(MAKE) -f Makefile.arm9 --no-print-directory
	@+$(MAKE) -f Makefile.arm9 --no-print-directory DEBUG=1

arm7:
	@+$(MAKE) -f Makefile.arm7 --no-print-directory
	@+$(MAKE) -f Makefile.arm7 --no-print-directory DEBUG=1

teak:
	@+$(MAKE) -f Makefile.teak --no-print-directory
	@+$(MAKE) -f Makefile.teak --no-print-directory DEBUG=1

clean:
	@echo "  CLEAN"
	@$(RM) $(VERSION_HEADER) lib build

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
