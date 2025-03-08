MACHINE = $(shell $(CC) -dumpmachine)

SRCDIR = $(realpath ../../src)
OBJDIR = $(realpath ../../output/$(MACHINE)/coverage)
INCDIR = $(realpath ../../include ../../include_priv ../common)
COMMON_SRCDIR = $(realpath ../common/)

HEADERS = $(wildcard $(INCDIR)/*.h)
SOURCES = $(wildcard $(SRCDIR)/*.c)
SOURCES += $(wildcard $(COMMON_SRCDIR)/*.c)

CFLAGS += -Werror -Wall -Wextra -Wno-attributes\
          --std=gnu99 -g3 -Wmissing-declarations \
		  $(addprefix -I ,$(INCDIR)) -I$(OBJDIR)/.. \
		  -fkeep-inline-functions -fkeep-static-functions \
		  -Wno-format-nonliteral \
		  $(shell pkg-config --cflags cmocka) -pthread -DUNIT_TEST \
		  -Wno-unused-variable -Wno-unused-parameter

LDFLAGS += -fkeep-inline-functions -fkeep-static-functions \
		   $(shell pkg-config --libs cmocka) \
		   -lamxc -lamxp -lamxd -lamxo \
		   -lamxb -lamxm -lnetmodel -lsahtrace \
		   -ldl -lpthread
