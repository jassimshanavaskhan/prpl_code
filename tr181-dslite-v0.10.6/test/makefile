PKG_CONFIG_LIBDIR := /usr/lib/pkgconfig:/lib/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig:$(PKG_CONFIG_LIBDIR)
MACHINE = $(shell $(CC) -dumpmachine)
SUBDIRS := $(wildcard tr181-dslite_*)
MODDIRS := $(wildcard common/mod-*)
OBJDIR = ../output/$(MACHINE)/coverage
COVERREPORT = report

run: $(OBJDIR)/
	@for dir in $(MODDIRS); do make -C $$dir || exit -1; done
	@for dir in $(SUBDIRS); do make -C $$dir $@  || exit -1; done
	@rm -rf $(OBJDIR)/test_* $(OBJDIR)/dummy_* $(OBJDIR)/mock* $(OBJDIR)/ares*
	@for dir in $(MODDIRS); do make -C $$dir clean || exit -1; done


clean: 
	rm -rf $(OBJDIR)
	rm -rf $(OBJDIR)/$(COVERREPORT)
	@for dir in $(MODDIRS); do make -C $$dir clean || exit -1; done
	find .. -name "run_test" -delete

coverage: $(OBJDIR)/$(COVERREPORT)/
	@cd $(OBJDIR) && \
	for i in $$(find . -type f -iname "*.o"); do \
		gcov -c -b -f --long-file-names --preserve-paths $$i > /dev/null; \
    done
	@cd $(OBJDIR) && for i in $$(find . -name "*.h.gcov"); do rm $$i > /dev/null; done
	cd $(OBJDIR) && gcovr -k -p -r ../../.. -s -g --html --html-details -o ./$(COVERREPORT)/index.html
	cd $(OBJDIR) && gcovr -k -p -r ../../.. -s -g | tee ./$(COVERREPORT)/gcovr_summary.txt

$(OBJDIR)/:
	mkdir -p $@

$(OBJDIR)/$(COVERREPORT)/:
	mkdir -p $@

.PHONY: run clean coverage