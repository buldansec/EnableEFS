SRC := $(wildcard *.c)
OUTDIR := build
CC_x86 := i686-w64-mingw32-gcc
CC_x64 := x86_64-w64-mingw32-gcc
STRIP_x86 := i686-w64-mingw32-strip
STRIP_x64 := x86_64-w64-mingw32-strip

CFLAGS_COMMON := -masm=intel -nostdlib -Wall -Wextra
CFLAGS_x64 := $(CFLAGS_COMMON)
CFLAGS_x86 := $(CFLAGS_COMMON) -DWOW64 -fno-leading-underscore

TARGETS := $(patsubst %.c,$(OUTDIR)/%.x64.o,$(SRC)) $(patsubst %.c,$(OUTDIR)/%.x86.o,$(SRC))

all: $(OUTDIR) $(TARGETS)

$(OUTDIR):
	@mkdir -p $(OUTDIR)

$(OUTDIR)/%.x64.o: %.c
	@$(CC_x64) -c $< -o $@ $(CFLAGS_x64)
	@$(STRIP_x64) --strip-unneeded $@

$(OUTDIR)/%.x86.o: %.c
	@$(CC_x86) -c $< -o $@ $(CFLAGS_x86)
	@$(STRIP_x86) --strip-unneeded $@
	
clean:
	rm -rf $(OUTDIR)
