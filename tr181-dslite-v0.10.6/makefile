include makefile.inc

NOW = $(shell date +"%Y-%m-%d(%H:%M:%S %z)")

# Extra destination directories
PKGDIR = ./output/$(MACHINE)/pkg/

define create_changelog
	@$(ECHO) "Update changelog"
	mv CHANGELOG.md CHANGELOG.md.bak
	head -n 9 CHANGELOG.md.bak > CHANGELOG.md
	$(ECHO) "" >> CHANGELOG.md
	$(ECHO) "## Release $(VERSION) - $(NOW)" >> CHANGELOG.md
	$(ECHO) "" >> CHANGELOG.md
	$(GIT) log --pretty=format:"- %s" $$($(GIT) describe --tags | grep -v "merge" | cut -d'-' -f1)..HEAD  >> CHANGELOG.md
	$(ECHO) "" >> CHANGELOG.md
	tail -n +10 CHANGELOG.md.bak >> CHANGELOG.md
	rm CHANGELOG.md.bak
endef

# targets
all:
	$(MAKE) -C src all
	$(MAKE) -C mod-dslite-ioctl/src all
	$(MAKE) -C odl all

clean:
	$(MAKE) -C src clean
	$(MAKE) -C mod-dslite-ioctl/src clean
	$(MAKE) -C mod-dslite-ioctl/test clean
	$(MAKE) -C test clean
	$(MAKE) -C odl clean

install: all
	$(INSTALL) -D -p -m 0644 output/$(MACHINE)/$(COMPONENT).so $(DEST)/usr/lib/amx/$(COMPONENT)/$(COMPONENT).so
	$(INSTALL) -D -p -m 0644 odl/$(COMPONENT).odl $(DEST)/etc/amx/$(COMPONENT)/$(COMPONENT).odl
	$(INSTALL) -D -p -m 0644 odl/$(COMPONENT)_definition.odl $(DEST)/etc/amx/$(COMPONENT)/$(COMPONENT)_definition.odl
	$(INSTALL) -D -p -m 0644 odl/dslite_defaults/dslite_defaults.odl $(DEST)/etc/amx/$(COMPONENT)/dslite_defaults/dslite_defaults.odl
	$(INSTALL) -D -p -m 0644 odl/dslite_defaults/dslite_interfacesettings.odl $(DEST)/etc/amx/$(COMPONENT)/dslite_defaults/dslite_interfacesettings.odl
	$(INSTALL) -D -p -m 0644 odl/$(COMPONENT)_mapping.odl $(DEST)/etc/amx/tr181-device/extensions/01_device-dslite_mapping.odl
	$(INSTALL) -D -p -m 0644 odl/$(COMPONENT)_caps.odl $(DEST)/etc/amx/$(COMPONENT)/$(COMPONENT)_caps.odl
	$(INSTALL) -D -p -m 0644 output/$(MACHINE)/mod-dslite-ioctl.so $(DEST)/usr/lib/amx/$(COMPONENT)/modules/mod-dslite-ioctl.so
	$(INSTALL) -D -p -m 0644 acl/$(COMPONENT).json $(DEST)/usr/share/acl.d/$(COMPONENT).json
	$(INSTALL) -d -m 0755 $(DEST)$(BINDIR)
	ln -sfr $(DEST)$(BINDIR)/amxrt $(DEST)$(BINDIR)/$(COMPONENT)
	$(INSTALL) -D -p -m 0755 scripts/$(COMPONENT).sh $(DEST)$(INITDIR)/$(COMPONENT)

package: all
	$(INSTALL) -D -p -m 0644 output/$(MACHINE)/$(COMPONENT).so $(PKGDIR)/usr/lib/amx/$(COMPONENT)/$(COMPONENT).so
	$(INSTALL) -D -p -m 0644 odl/$(COMPONENT).odl $(PKGDIR)/etc/amx/$(COMPONENT)/$(COMPONENT).odl
	$(INSTALL) -D -p -m 0644 odl/$(COMPONENT)_definition.odl $(PKGDIR)/etc/amx/$(COMPONENT)/$(COMPONENT)_definition.odl
	$(INSTALL) -D -p -m 0644 odl/dslite_defaults/dslite_defaults.odl $(PKGDIR)/etc/amx/$(COMPONENT)/dslite_defaults/dslite_defaults.odl
	$(INSTALL) -D -p -m 0644 odl/dslite_defaults/dslite_interfacesettings.odl $(PKGDIR)/etc/amx/$(COMPONENT)/dslite_defaults/dslite_interfacesettings.odl
	$(INSTALL) -D -p -m 0644 odl/$(COMPONENT)_mapping.odl $(PKGDIR)/etc/amx/tr181-device/extensions/01_device-dslite_mapping.odl
	$(INSTALL) -D -p -m 0644 odl/$(COMPONENT)_caps.odl $(PKGDIR)/etc/amx/$(COMPONENT)/$(COMPONENT)_caps.odl
	$(INSTALL) -D -p -m 0644 output/$(MACHINE)/mod-dslite-ioctl.so $(PKGDIR)/usr/lib/amx/$(COMPONENT)/modules/mod-dslite-ioctl.so
	$(INSTALL) -D -p -m 0644 acl/$(COMPONENT).json $(PKGDIR)/usr/share/acl.d/$(COMPONENT).json
	$(INSTALL) -d -m 0755 $(PKGDIR)$(BINDIR)
	rm -f $(PKGDIR)$(BINDIR)/$(COMPONENT)
	ln -sfr $(PKGDIR)$(BINDIR)/amxrt $(PKGDIR)$(BINDIR)/$(COMPONENT)
	$(INSTALL) -D -p -m 0755 scripts/$(COMPONENT).sh $(PKGDIR)$(INITDIR)/$(COMPONENT)
	cd $(PKGDIR) && $(TAR) -czvf ../$(COMPONENT)-$(VERSION).tar.gz .
	cp $(PKGDIR)../$(COMPONENT)-$(VERSION).tar.gz .
	make -C packages

changelog:
	$(call create_changelog)

doc:
	$(eval ODLFILES += odl/$(COMPONENT).odl)
	$(eval ODLFILES += odl/$(COMPONENT)_definition.odl)
	$(eval ODLFILES += odl/dslite_defaults/dslite_defaults.odl)
	$(eval ODLFILES += odl/dslite_defaults/dslite_interfacesettings.odl)
	$(eval ODLFILES += odl/$(COMPONENT)_mapping.odl)
	$(eval ODLFILES += odl/$(COMPONENT)_caps.odl)

	mkdir -p output/xml
	mkdir -p output/html
	mkdir -p output/confluence
	amxo-cg -Gxml,output/xml/$(COMPONENT).xml $(or $(ODLFILES), "")
	amxo-xml-to -x html -o output-dir=output/html -o title="$(COMPONENT)" -o version=$(VERSION) -o sub-title="Datamodel reference" output/xml/*.xml
	amxo-xml-to -x confluence -o output-dir=output/confluence -o title="$(COMPONENT)" -o version=$(VERSION) -o sub-title="Datamodel reference" output/xml/*.xml

test:
	$(MAKE) -C test run
	$(MAKE) -C test coverage
	$(MAKE) -C mod-dslite-ioctl/test run
	$(MAKE) -C mod-dslite-ioctl/test coverage

.PHONY: all clean changelog install package doc test