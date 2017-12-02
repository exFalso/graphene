manifests = sqlite3.manifest
SRCDIR = sqlite-3.21

target = build-sqlite
exec_target = $(manifests)

level = ../../
include ../../Makefile

build-sqlite: build/bin/sqlite3 $(addsuffix .so,$(iconvdata))

build/bin/sqlite3: $(SRCDIR)
	cd $(SRCDIR) && ./configure --prefix=$(PWD)/build
	$(MAKE) -C $(SRCDIR)
	$(MAKE) -C $(SRCDIR) install

build/lib:
	mkdir -p $@

$(SRCDIR): %: %.tar.gz
	tar -xzf $<

distclean: clean
	rm -rf build $(SRCDIR)
