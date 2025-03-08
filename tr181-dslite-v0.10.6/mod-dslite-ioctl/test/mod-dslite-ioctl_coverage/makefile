include ../test_defines.mk

$(info $$OBJDIR is [${OBJDIR}])

TARGET = run_test
TARGET_MODULE = ../modules_under_test/target_module.so
SOURCES += $(wildcard ./*.c)
OBJECTS = $(addprefix $(OBJDIR)/,$(notdir $(SOURCES:.c=.o))) 

include ../test_targets.mk
