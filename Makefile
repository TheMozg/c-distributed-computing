TARGET   = pa1

CC       = clang
CFLAGS   = -std=c99 -Wall -pedantic

SRCDIR   = pa1
BINDIR   = bin
TARDIR   = tar

SOURCES  := $(wildcard $(SRCDIR)/*.c)
HEADERS  := $(wildcard $(SRCDIR)/*.h)

PACKAGE  := $(TARGET).tar.gz

.PHONY: all
all: $(TARDIR)/$(PACKAGE)

$(BINDIR)/$(TARGET): $(SOURCES) $(HEADERS) $(MAKEFILE_LIST) | $(BINDIR)
	$(CC) $(CFLAGS) $(SOURCES) -o $@

$(TARDIR)/$(PACKAGE): $(BINDIR)/$(TARGET) | $(TARDIR)
	tar czf $(TARDIR)/$(PACKAGE) $(SOURCES) $(HEADERS)

$(BINDIR) $(TARDIR):
	mkdir $@

.PHONY: clean
clean:
	rm -rf $(BINDIR)
	rm -rf $(TARDIR)
