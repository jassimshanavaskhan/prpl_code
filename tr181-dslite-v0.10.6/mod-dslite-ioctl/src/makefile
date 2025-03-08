include ../../makefile.inc

# build destination directories
OBJDIR = ../../output/$(MACHINE)

# TARGETS
MODULE = mod-dslite-ioctl
TARGET_SO = $(OBJDIR)/$(MODULE).so

# directories
# source directories
SRCDIR = .
INCDIR_PRIV = ../include_priv
INCDIRS = $(INCDIR_PRIV) $(if $(STAGINGDIR), $(STAGINGDIR)/include $(STAGINGDIR)/usr/include)
STAGING_LIBDIR = $(if $(STAGINGDIR), -L$(STAGINGDIR)/lib) $(if $(STAGINGDIR), -L$(STAGINGDIR)/usr/lib)

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(addprefix $(OBJDIR)/,$(notdir $(SOURCES:.c=.o)))

# compilation and linking flags
CFLAGS += -Werror -Wall -Wextra \
		  -Wformat=2 -Wshadow \
		  -Wwrite-strings -Wredundant-decls \
		  -Wmissing-declarations -Wno-attributes \
		  -Wno-format-nonliteral \
		  -DSAHTRACES_ENABLED -DSAHTRACES_LEVEL_DEFAULT=500 \
		  -fPIC -g3 $(addprefix -I ,$(INCDIRS))

ifeq ($(CC_NAME),g++)
	CFLAGS += -std=c++2a
else
	CFLAGS += -Wstrict-prototypes -Wold-style-definition -Wnested-externs -std=gnu11
endif

LDFLAGS += -shared -fPIC $(STAGING_LIBDIR) \
		   -lamxc -lamxm  -lsahtrace

# targets
all: $(TARGET_SO)

$(TARGET_SO): $(OBJECTS)
	$(CC) -Wl,-soname,$(MODULE).so -o $@ $(OBJECTS) $(LDFLAGS)

-include $(OBJECTS:.o=.d)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)/
	$(CC) $(CFLAGS) -c -o $@ $<
	@$(CC) $(CFLAGS) -MM -MP -MT '$(@) $(@:.o=.d)' -MF $(@:.o=.d) $(<)

$(OBJDIR)/:
	$(MKDIR) -p $@

clean:
	rm -rf ../output/ ../$(MODULE)-*.* ../$(MODULE)_*.*

.PHONY: all clean
