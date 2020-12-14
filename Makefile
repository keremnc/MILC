CC=gcc
DEPS=include/milc.h
VPATH=src:test
BUILDDIR=target
BINDIR=$(BUILDDIR)/bin
OBJDIR=$(BUILDDIR)/tmp

OBJS=$(patsubst %.o,$(OBJDIR)/%.o,milc_v1.o milc_v2.o milc_v3.o milc_v4.o milc_v5.o simdized.o)

all: $(BINDIR)/timing $(BINDIR)/validation

$(OBJDIR)/%.o: %.c $(DEPS) | $(OBJDIR)
	$(CC) -Iinclude -march=skylake-avx512 -c -o $@ $<

$(BINDIR)/timing: $(OBJS) $(OBJDIR)/timing.o | $(BINDIR)
	$(CC) -o $@ $(OBJS) $(OBJDIR)/timing.o -lm -O3

$(BINDIR)/validation: $(OBJS) $(OBJDIR)/validation.o | $(BINDIR)
	$(CC) -o $@ $(OBJS) $(OBJDIR)/validation.o -lm -O3	

$(BINDIR) $(OBJDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILDDIR)
