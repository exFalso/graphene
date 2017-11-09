SYS ?= $(shell gcc -dumpmachine)

LAT_PROC_ENCLAVE_SIZE ?= 128M
LAT_THREAD_MAX ?= 16

CC	= gcc
CFLAGS	= -Wall -O2 -std=gnu99 -fgnu89-inline -fno-builtin -nostdlib \
	  -I../include/pal -I../lib \
	  -fno-omit-frame-pointer \
	  -fno-stack-protector -fno-builtin

default: all
include ../src/Makefile.Test

testfiles = /tmp/XXX \
	    /tmp/YYY/YYY/XXX \
	    /tmp/YYY/YYY/YYY/YYY/XXX \
	    /tmp/YYY/YYY/YYY/YYY/YYY/YYY/XXX
lib_files = lib_timing.c lib_confidence.c sqrt.c
executables = $(patsubst %.c,%,$(filter-out $(lib_files),$(wildcard *.c)))
manifests = manifest $(patsubst %.template,%,$(wildcard *.manifest.template))

target = $(executables)

graphene_lib = .lib/graphene-lib.a
pal_lib = ../src/libpal.so
headers = $(wildcard ../include/pal/*.h) $(wildcard *.h)

all:	pal_loader $(manifests) $(call expand_target,$(executables))

ifeq ($(DEBUG),1)
CC += -g
endif
export DEBUG

pal_loader:
	ln -sf ../../Runtime/pal_loader

$(manifests): %: %.template $(testfiles)
	sed \
	-e 's:\$$(LAT_PROC_ENCLAVE_SIZE):$(LAT_PROC_ENCLAVE_SIZE):g' \
	-e 's:\$$(LAT_THREAD_MAX):$(LAT_THREAD_MAX):g' \
	$< > $@

ifeq ($(SYS),x86_64-linux-gnu)
$(executables): %: %.c $(patsubst %.c,%.o,$(lib_files)) ../src/user_start.o $(graphene_lib) $(pal_lib) $(headers)
	@echo [ $@ ]
	@$(CC) $(CFLAGS) $(filter-out $(headers),$^) -o $@

%.o: %.c $(headers)
	@echo [ $@ ]
	@$(CC) $(CFLAGS) -c $< -o $@

$(graphene_lib):
	$(MAKE) -C ../lib target=$(shell pwd)/.lib/

.PHONY: pack
pack: $(executables)
	@../../Scripts/pack_binaries.sh test $^
else
$(executables): .packed/benchmark.tar.gz
	tar -xmozf $< $@
endif

$(testfiles):
	mkdir -p $(dir $@)
	dd if=/dev/zero of=$@ bs=1MiB count=1

test: all $(testfiles)
	env LOADER=./pal_loader ./run-tests

clean:
	rm -rf $(patsubst %.c,%.o,$(lib_files))
	rm -rf pal_loader $(call expand_target,$(target)) .lib *.cached
