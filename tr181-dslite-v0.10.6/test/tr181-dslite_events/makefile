include ../test_defines.mk


TARGET = run_test
SOURCES += $(wildcard ./*.c)
OBJECTS = $(addprefix $(OBJDIR)/,$(notdir $(SOURCES:.c=.o))) 

WRAP_FUNC=-Wl,--wrap=
MOCK_WRAP = amxb_be_who_has \
			netmodel_openQuery_getAddrs \
          	netmodel_closeQuery \
          	netmodel_initialize \
			netmodel_openQuery_getDHCPOption \
			amxo_connection_add \
			amxb_set

LDFLAGS += -g $(addprefix $(WRAP_FUNC),$(MOCK_WRAP))

include ../test_targets.mk