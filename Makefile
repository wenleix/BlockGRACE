#
# Simple Makefile for GRACE
#
# Guozhang Wang 1/19/2012
#
#

PKGNAME := GRACE

ifndef CUDB_PREFIX
  $(error CUDB_PREFIX does not exist)
endif

include MakefileCommon

################################################################################

#CXX := $(shell which c++)
CXX := g++
CXXFLAGS = -O3 #-ffast-math
DEFFLAGS = -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -DDBUTL_NDEBUG #-DMEMORY_BARRIER

################################################################################
#
# List of all targets ...
#

.PHONY : all
all: $(BIN)/GRACE.$(CMD_EXT)

#
################################################################################

grace_NAMES := Driver Scheduler GraphBase Graph Job TaskBase Task GraphReader Profile GraphBlock Util SparseScheduler 
LIB_NAMES := -lDBUtl -lpthread -lrt

$(eval $(call GrpBEGIN,$(SRC)))

$(eval $(call GrpADD4CMD,$(grace_NAMES)))

$(eval $(call GrpADD4CMD,main))

$(eval $(call GrpMKCMD,GRACE,$(LIB_NAMES),$(grace_NAMES) main))

$(eval $(call GrpEND))




