#
# This is the makefile building your odl files
#

-include ../makefile.inc

# Set default values if CONFIG_SAH_AMX_TR181_DSLITE_RUN_AS_USER or CONFIG_SAH_AMX_TR181_DSLITE_RUN_AS_GROUP is empty
ifeq ($(strip $(CONFIG_SAH_AMX_TR181_DSLITE_RUN_AS_USER)),)
    CONFIG_SAH_AMX_TR181_DSLITE_RUN_AS_USER := "root"
endif

ifeq ($(strip $(CONFIG_SAH_AMX_TR181_DSLITE_RUN_AS_GROUP)),)
    CONFIG_SAH_AMX_TR181_DSLITE_RUN_AS_GROUP := "root"
endif

all:
	m4 -D USER_ID=$(CONFIG_SAH_AMX_TR181_DSLITE_RUN_AS_USER) -D GROUP_ID=$(CONFIG_SAH_AMX_TR181_DSLITE_RUN_AS_GROUP) tr181-dslite_caps.odl.m4 > tr181-dslite_caps.odl

clean:
	rm -rf tr181-dslite_caps.odl

#
# All these targets must be run regardless of the presence of a file
# named after them.
#
.PHONY: all clean


