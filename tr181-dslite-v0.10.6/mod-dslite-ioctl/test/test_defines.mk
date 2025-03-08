MACHINE = $(shell $(CC) -dumpmachine)

SRCDIR = $(realpath ../../src)
OBJDIR = $(realpath ../../output/$(MACHINE)/mod_coverage)
INCDIR = $(realpath ../../include ../../include_priv ../include)
MOCK_SRCDIR = $(realpath ../common/)

HEADERS = $(wildcard $(INCDIR)/*.h)
SOURCES = $(wildcard $(SRCDIR)/*.c) \
		  $(wildcard $(MOCK_SRCDIR)/*.c)


CFLAGS += -Werror -Wall -Wextra -Wno-attributes\
		  --std=gnu99 -fPIC -g3 -Wmissing-declarations \
		  $(addprefix -I ,$(INCDIR)) -I$(OBJDIR)/.. \
		  -fkeep-inline-functions -fkeep-static-functions \
		   -Wno-format-nonliteral \
		  $(shell pkg-config --cflags cmocka) -pthread -DUNIT_TEST
		  
LDFLAGS += -fkeep-inline-functions -fkeep-static-functions \
		   $(shell pkg-config --libs cmocka) \
		   -lamxb -lamxc -lamxd -lamxm -lamxo -lamxp -ldl -lsahtrace -lpthread