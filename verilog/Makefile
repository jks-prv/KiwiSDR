#
# When running Vivado on a machine/OS different from where the Kiwi sources are kept,
# and those two machines are connected via a shared folder, specify the shared folder directory
# with V_SRC_DIR and use the makefile targets "cv" and "cb" to copy sources and results back and forth.
#
# When building in batch mode, as opposed to using the Vivado UI, V_SRC_DIR is passed to the tcl script
# to specify where to place the resulting .bit files.
#
# V_SRC_DIR can be set as a global environment variable otherwise it will default to the value below.
#

# Copyright (c) 2014-2026 John Seamons, ZL4VO/KF6VO

# include any rules from another FPGA project using the KiwiSDR framework (i.e. CFG = other)
-include $(wildcard */Makefile.inc)

V_PROJ    ?= KiwiSDR
V_SRC_DIR ?= /media/sf_shared

# name of verilog top level design is the same between projects
V_TOP ?= KiwiSDR

ifeq ($(DEBIAN_DEVSYS),)
    DEBIAN_DEVSYS = verilog_host
endif

IP_VER = 2022.2
#IP_VER = 2024.2

ifeq ($(DEBIAN_DEVSYS),$(DEVSYS))

################################
# devsys
################################

V_DIR ?= ~/sf_shared

$(GEN_VERILOG): $(GEN_DIR)/kiwi.gen.h verilog/rx/cic_gen.c
	(cd verilog/rx; make)

# generate the files needed to build the Verilog code
verilog: $(GEN_VERILOG)
	@echo "verilog/ directory should now contain all necessary generated files:"
	@echo "verilog/kiwi.gen.vh, verilog/rx/cic_*.vh"

# command to "copy verilog" from KiwiSDR distribution to the Vivado build location
# designed to complement the "make cv" command run on the Vivado build machine
EXCLUDE_CV = ".DS_Store" "rx/cic_gen" "rx/*.dSYM" "*(original)*"

# cv2_dev is a rule that can be defined in */Makefile.inc (i.e. CFG = other) to do additional source installation
cv2_dev:

cv: $(GEN_VERILOG)
	@echo "remember to define DOWNLOAD_BIT (via dlb[yn]) so copyback of binaries works"
	@echo "V_PROJ = $(V_PROJ)"
	@echo "V_DIR = $(V_DIR)"
	@echo "RX_CFG  = $(shell grep RX_CFG  verilog/kiwi.cfg.vh | cut -d\  -f 4 | tr -d ';')"
	@echo "MODE_ID = $(shell grep MODE_ID verilog/kiwi.cfg.vh | cut -d\  -f 4 | tr -d ';')"
	rsync -av --delete $(addprefix --exclude , $(EXCLUDE_CV)) verilog/ $(V_DIR)/$(V_PROJ)
	rsync -av --delete $(addprefix --exclude , $(EXCLUDE_CV)) verilog.Vivado.$(IP_VER).ip/ $(V_DIR)/$(V_PROJ).Vivado.$(IP_VER).ip
	make cv2_dev V_SRC_DIR=$(V_SRC_DIR)

sum: $(BIN_DEPS)
	sum *.bit *.aout

endif


ifeq ($(DEBIAN_DEVSYS),$(DEBIAN))
sum: $(BIN_DEPS)
	@sum KiwiSDR.rx*.bit
	@echo
	@sum KiwiSDR_a50.rx*.bit
	@echo
	@sum *.aout
endif


ifeq ($(DEBIAN_DEVSYS),verilog_host)

################################
# Vivado build host
################################

# when using a different FPGA part it is best to use an entirely separate project
V_PROJ1 ?= KiwiSDR
V_PROJ2 ?= KiwiSDR_a50

ifeq ($(findstring a50,$(CURDIR)),)
    V_PROJ := $(V_PROJ1)
    PART   := xc7a35tftg256-1
else
    V_PROJ := $(V_PROJ2)
    PART   := xc7a50tftg256-1
endif

RUN_DIR     := /home/jks/verilog
V_DST_DIR   := $(RUN_DIR)/$(V_PROJ)/import_srcs
V_DST_DIR1  := $(RUN_DIR)/$(V_PROJ1)/import_srcs
V_DST_DIR2  := $(RUN_DIR)/$(V_PROJ2)/import_srcs

V_BASE  := time vivado -mode batch -source make_proj.tcl -tclargs --result_dir $(V_SRC_DIR)
V_BATCH := $(V_BASE) --origin_dir $(V_DST_DIR) --part $(PART) --project_name $(V_PROJ)

all:
	@reset
	@echo "build project with Vivado via tcl"
	@echo "$(V_BATCH)"
	(cd $(RUN_DIR); $(V_BATCH))

rx83:
	@reset
	@echo "build rx8_wf3 project with Vivado via tcl"
	(cd $(RUN_DIR); $(V_BATCH) --rx8_wf3)

rx44:
	@reset
	@echo "build rx4_wf4 project with Vivado via tcl"
	(cd $(RUN_DIR); $(V_BATCH) --rx4_wf4)

rx82:
	@reset
	@echo "build rx8_wf2 project with Vivado via tcl"
	(cd $(RUN_DIR); $(V_BATCH) --rx8_wf2)

rx33:
	@reset
	@echo "build rx3_wf3 project with Vivado via tcl"
	(cd $(RUN_DIR); $(V_BATCH) --rx3_wf3)

rx14:
	@reset
	@echo "build rx14_wf0 project with Vivado via tcl"
	(cd $(RUN_DIR); $(V_BATCH) --rx14_wf0)

rx1:
	@reset
	@echo "build rx1_wf0 project with Vivado via tcl"
	(cd $(RUN_DIR); $(V_BATCH) --rx1_wf0)

rebuild_ip:
	@reset
	@echo "rebuild project ip with Vivado via tcl"
	@echo "7/2025 NB: With Vivado 2024.2 this doesn't seem to be working currently."
	rm -rf $(V_PROJ) $(RUN_DIR)/generated
	mkdir -p $(RUN_DIR)/generated
	(cd $(RUN_DIR); $(V_BATCH) --regen_ip)

# command to "copy verilog" from KiwiSDR distribution into our build directory
# designed to complement the "make cv" command run in the KiwiSDR server build directory
EXCLUDE_RSYNC = ".DS_Store" "KiwiSDR/" "generated" ".Xil/" "vivado.*"

# cv2_host is a rule that can be defined in */Makefile.inc (i.e. CFG = other) to do additional source installation
cv2_host:

# copies same sources files to both projects
cv:
	@echo "V_TOP  = $(V_TOP)"
	@echo "V_PROJ  = $(V_PROJ)"
	@echo "V_SRC_DIR = $(V_SRC_DIR)"
	@echo "V_DST_DIR1 = $(V_DST_DIR1)"
	@echo "V_DST_DIR2 = $(V_DST_DIR2)"

	rsync -av --delete $(addprefix --exclude , $(EXCLUDE_RSYNC)) $(V_SRC_DIR)/$(V_TOP)/ $(V_DST_DIR1)
	rsync -av --delete $(addprefix --exclude , $(EXCLUDE_RSYNC)) $(V_SRC_DIR)/$(V_TOP)/ $(V_DST_DIR2)
	rsync -av --delete --exclude .DS_Store $(V_SRC_DIR)/$(V_TOP).Vivado.$(IP_VER).ip/ $(V_DST_DIR1)/../import_ip
	rsync -av --delete --exclude .DS_Store $(V_SRC_DIR)/$(V_TOP).Vivado.$(IP_VER).ip/ $(V_DST_DIR2)/../import_ip
	cp kiwi.tcl make_proj.tcl $(RUN_DIR)
	make cv2_host V_SRC_DIR=$(V_SRC_DIR)

RX_CFG  := $(shell grep RX_CFG  kiwi.cfg.vh | cut -d\  -f 4 | tr -d ';')
MODE_ID := $(shell grep MODE_ID kiwi.cfg.vh | cut -d\  -f 4 | tr -d ';')

cb:
	@echo "V_TOP  = $(V_TOP)"
	@echo "V_PROJ = $(V_PROJ)"
	@echo "MODE_ID = $(MODE_ID)"
	@echo "V_DST_DIR = $(V_DST_DIR)"

	cp $(V_DST_DIR)/../*.runs/impl_1/$(V_TOP).bit $(V_SRC_DIR)/$(V_PROJ).$(MODE_ID).bit
	sum $(V_SRC_DIR)/$(V_PROJ).*.bit

sum:
	sum $(V_SRC_DIR)/*.bit

lt:
	ls -lt $(V_SRC_DIR)

mu:
	-more -f ../$(V_PROJ).runs/impl_1/utilization_routed.rpt

ts:
	-tail -f $(V_PROJ)/$(V_PROJ).runs/synth_1/runme.log

cs:
	cat $(V_PROJ)/$(V_PROJ).runs/synth_1/runme.log

ti:
	-tail -f $(V_PROJ)/$(V_PROJ).runs/impl_1/runme.log

ci:
	cat $(V_PROJ)/$(V_PROJ).runs/impl_1/runme.log

RSYNC_REV_INC = $(addprefix --include=, "*.veo" "*.xci" "*.xml")
cip_rev:
	rsync -av --delete-excluded --prune-empty-dirs --include "*/"  $(RSYNC_REV_INC) --exclude="*" ../$(V_PROJ).srcs/sources_1/ip/ ../import_ip/
	rsync -av --delete --exclude .DS_Store ../import_ip/ $(V_SRC_DIR)/$(V_TOP).Vivado.$(IP_VER).ip

cip_cp:
	rsync -av --delete --exclude .DS_Store ipcore_properties $(V_SRC_DIR)/$(V_TOP).Vivado.$(IP_VER).ipcore_properties

cm:
	cp kiwi_map.mrp kiwi_map_mrp.html $(V_SRC_DIR)
	cp $(V_PROJ).syr kiwi_syr.html $(V_SRC_DIR)
	cp $(V_PROJ).twr kiwi_twr.html $(V_SRC_DIR)
	cp $(V_PROJ).twx $(V_SRC_DIR)

cp:
	cp $(V_PROJ).pwr $(V_SRC_DIR)

clean:
	rm -f kiwi.coe

cbash:
	cp $(V_SRC_DIR)/bash_aliases /home/jks/.bash_aliases
	cp $(V_SRC_DIR)/bashrc.local /home/jks/.bashrc.local

endif

proj:
	@echo "V_PROJ  = $(V_PROJ)"
	@echo "V_PROJ2 = $(V_PROJ2)"
