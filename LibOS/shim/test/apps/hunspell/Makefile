iconvdata = ISO8859-1
manifests = hunspell.manifest
SRCDIR = hunspell-1.6.2

target = build-hunspell
exec_target = $(manifests)

level = ../../
include ../../Makefile

build-hunspell: build/bin/hunspell $(addsuffix .so,$(iconvdata))

build/bin/hunspell: $(SRCDIR)
	cd $(SRCDIR) && autoreconf -vfi
	cd $(SRCDIR) && ./configure --prefix=$(PWD)/build
	$(MAKE) -C $(SRCDIR)
	$(MAKE) -C $(SRCDIR) install

build/lib:
	mkdir -p $@

$(addsuffix .so,$(iconvdata)): %: ../../../../glibc-build/iconvdata/% build/lib
	ln -sf $< $@

$(SRCDIR): %: %.tar.gz
	tar -xzf $<

distclean: clean
	rm -rf build $(SRCDIR)
