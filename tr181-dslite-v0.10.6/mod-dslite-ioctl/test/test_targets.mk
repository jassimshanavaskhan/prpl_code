all: $(TARGET)

run: $(TARGET) $(TARGET_MODULE)
	set -o pipefail; valgrind --leak-check=full --exit-on-first-error=yes --error-exitcode=1 ./$< 2>&1 | tee -a $(OBJDIR)/unit_test_results.txt;

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) -fprofile-arcs -ftest-coverage

$(TARGET_MODULE): $(OBJECTS)
	$(CC) -shared $(OBJECTS) -fprofile-arcs -ftest-coverage -Wl,-soname,$(@) -o $@ $(LDFLAGS)

-include $(OBJECTS:.o=.d)

$(OBJDIR)/%.o: ./%.c | $(OBJDIR)/
	$(CC) $(CFLAGS) -fprofile-arcs -ftest-coverage -c -o $@ $<
	@$(CC) $(CFLAGS) -MM -MP -MT '$(@) $(@:.o=.d)' -MF $(@:.o=.d) $(<)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)/
	$(CC) $(CFLAGS) -fprofile-arcs -ftest-coverage -c -o $@ $<
	@$(CC) $(CFLAGS) -MM -MP -MT '$(@) $(@:.o=.d)' -MF $(@:.o=.d) $(<)

$(OBJDIR)/%.o: $(MOCK_SRCDIR)/%.c | $(OBJDIR)/
	$(CC) $(CFLAGS) -shared -Wl,--wrap=ioctl -fprofile-arcs -ftest-coverage -c -o $@ $<
	@$(CC) $(CFLAGS) -MM -MP -MT '$(@) $(@:.o=.d)' -MF $(@:.o=.d) $(<)

$(OBJDIR)/:
	mkdir -p $@

clean:
	rm -f $(TARGET) $(OBJDIR)
	rm -f $(TARGET_MODULE) $(OBJDIR)

.PHONY: clean $(OBJDIR)/