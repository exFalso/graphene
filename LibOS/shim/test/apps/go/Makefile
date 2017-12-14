GO_DIR = go-1.9.2
GOBOOTSTRAP_DIR = go-1.4
GOBOOTSTRAP_TAR = go1.4.3.linux-amd64.tar.gz
GOBOOTSTRAP_URL = https://storage.googleapis.com/golang

CFLAGS=-g
export CFLAGS

examples = examples/hello.go
examplebins = $(patsubst examples/%.go,%,$(examples))
manifests = go.manifest go-local.manifest manifest

target =
exec_target = $(manifests) $(examplebins)

extra_rules = -e 's:\$$(GODIR):$(GO_DIR):g'
clean-extra += clean-tmp

level = ../../
include ../../Makefile

$(GO_DIR)/bin/go: $(GO_DIR)/src/make.bash $(GOBOOTSTRAP_DIR)/bin/go
	cd $(GO_DIR)/src && \
		GOROOT_BOOTSTRAP=$(abspath $(GOBOOTSTRAP_DIR)) ./make.bash --no-banner

$(GOBOOTSTRAP_DIR)/bin/go:
	wget $(GOBOOTSTRAP_URL)/$(GOBOOTSTRAP_TAR)
	tar -xzf $(GOBOOTSTRAP_TAR)
	mv go $(GOBOOTSTRAP_DIR)

$(GO_DIR)/src/make.bash: $(GO_DIR).tar.gz
	tar -xzf $<

$(GO_DIR)/pkg/linux_amd64_dynlink/libruntime,sync-atomic.so: $(GO_DIR)/bin/go
	$(GO_DIR)/bin/go install -a -x -buildmode=shared runtime sync/atomic

$(examplebins): %: examples/%.go $(GO_DIR)/pkg/linux_amd64_dynlink/libruntime,sync-atomic.so
	$(GO_DIR)/bin/go build -linkshared $<

clean-tmp:
	rm -f *.sgx *.sig *.token

distclean: clean
	rm -rf $(GO_DIR) $(GOBOOTSTRAP_DIR) $(GOBOOTSTRAP_TAR)
