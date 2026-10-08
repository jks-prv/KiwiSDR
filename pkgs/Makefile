# generate calls to packages init routines
#
# these come from the root Makefile:
#	$(PKGS_INIT) contains a list of all packages needing init

# Copyright (c) 2026 John Seamons, ZL4VO/KF6VO

# NB: BUILD_DIR only goes up one level because this Makefile is directly included by ../Makefile
BUILD_DIR = ../build
GEN_DIR = $(BUILD_DIR)/gen

PKGS_FILE = $(GEN_DIR)/pkgs_init.cpp
PKGS_CALL = "\textern void $(pkg_name)_init(); $(pkg_name)_init();\n"

$(PKGS_FILE): pkgs/Makefile
	@echo generating $(PKGS_FILE)
	@echo PKGS_INIT = $(PKGS_INIT)
	@echo "// auto-generated file -- do not edit by hand" > $(PKGS_FILE)
	
	@echo "void pkgs_init() {" >> $(PKGS_FILE)
	@echo $(foreach pkg_name,$(PKGS_INIT),$(PKGS_CALL)) >> $(PKGS_FILE)1
	@sort -bdf -f $(PKGS_FILE)1 >> $(PKGS_FILE)
	@rm -f $(PKGS_FILE)1
	@echo "}\n" >> $(PKGS_FILE)
