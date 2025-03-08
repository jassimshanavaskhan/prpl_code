include ../makefile.inc

# build destination directories
OUTPUTDIR = ../output/$(MACHINE)
OBJDIR = $(OUTPUTDIR)

# TARGETS
TARGET_SO = $(OBJDIR)/$(COMPONENT).so

# directories
# source directories
SRCDIR = .
INCDIR_PUB =
INCDIR_PRIV = ../include_priv
INCDIRS = $(INCDIR_PUB) $(INCDIR_PRIV)
STAGING_LIBDIR =

ifeq ($(CONFIG_SAH_AMX_TR181_DSLITE_CARES_RPATH),y)
	STAGING_LIBDIR += -L$(STAGINGDIR)/opt/prplos/usr/lib
	INCDIRS += $(if $(STAGINGDIR), $(STAGINGDIR)/opt/prplos/usr/include)
endif
INCDIRS += $(if $(STAGINGDIR), $(STAGINGDIR)/include) $(if $(STAGINGDIR), $(STAGINGDIR)/usr/include)
STAGING_LIBDIR += $(if $(STAGINGDIR), -L$(STAGINGDIR)/lib) $(if $(STAGINGDIR), -L$(STAGINGDIR)/usr/lib)

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(addprefix $(OBJDIR)/,$(notdir $(SOURCES:.c=.o)))

# compilation and linking flags
CFLAGS += -Werror -Wall -Wextra \
          -Wformat=2 -Wshadow \
          -Wwrite-strings -Wredundant-decls \
		  -Wmissing-declarations -Wno-attributes \
		  -Wno-format-nonliteral \
		  -fPIC -g3 $(addprefix -I ,$(INCDIRS)) \
		  -DSAHTRACES_ENABLED -DSAHTRACES_LEVEL_DEFAULT=500 \
		  -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L

ifeq ($(CC_NAME),g++)
    CFLAGS += -std=c++2a
else
	CFLAGS += -Wstrict-prototypes -Wold-style-definition -Wnested-externs -std=c11
endif

LDFLAGS += $(STAGING_LIBDIR) -shared -fPIC \
           -lamxc -lamxp -lamxd -lamxo -lamxm -lnetmodel -lcares

# targets
all: $(TARGET_SO)

$(TARGET_SO): $(OBJECTS)
ifeq ($(CONFIG_SAH_AMX_TR181_DSLITE_CARES_RPATH),y)
	echo "!!! Compiling with --rpath to /opt/prplos/usr/lib/ !!!"
	$(CC) -Wl,-rpath=/opt/prplos/usr/lib/,-soname,$(COMPONENT).so -o $@ $(OBJECTS) $(LDFLAGS)
else
	$(CC) -Wl,-soname,$(COMPONENT).so -o $@ $(OBJECTS) $(LDFLAGS)
endif

-include $(OBJECTS:.o=.d)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)/
	$(CC) $(CFLAGS) -c -o $@ $<
	@$(CC) $(CFLAGS) -MM -MP -MT '$(@) $(@:.o=.d)' -MF $(@:.o=.d) $(<)

$(OBJDIR)/:
	$(MKDIR) -p $@

clean:
	rm -rf ../output/ ../$(COMPONENT)-*.* ../$(COMPONENT)_*.*

.PHONY: all clean
