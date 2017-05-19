CC       = clang
CFLAGS   = -std=gnu99 -Wall -pedantic

ifeq ($(DEBUG_IPC),y)
   CFLAGS += -D_DEBUG_IPC_
endif

ifeq ($(DEBUG_PA),y)
   CFLAGS += -D_DEBUG_PA_
endif

BINDIR   = bin
TARDIR   = tar

.PHONY: all
all:

$(BINDIR) $(TARDIR):
	mkdir $@

define INIT_PROJECT_RULES
SOURCES_$(1)  := $$(wildcard $(1)/*.c)
HEADERS_$(1)  := $$(wildcard $(1)/*.h)
PACKAGE_$(1)  := $(TARDIR)/$(1).tar.gz
TARGET_$(1)   := $(BINDIR)/$(1)
CFLAGS_$(1)   := $(2)


all: $(1)

.PHONY: $(1)
$(1): $$(PACKAGE_$(1))

$$(PACKAGE_$(1)): $$(TARGET_$(1)) | $(TARDIR)
	tar czf $$(PACKAGE_$(1)) $$(SOURCES_$(1)) $$(HEADERS_$(1))

$$(TARGET_$(1)): $$(SOURCES_$(1)) $$(HEADERS_$(1)) $(MAKEFILE_LIST) | $(BINDIR)
	$(CC) $(CFLAGS) $$(CFLAGS_$(1)) $$(SOURCES_$(1)) -o $$@

endef

$(eval $(call INIT_PROJECT_RULES,pa1))
$(eval $(call INIT_PROJECT_RULES,pa2,-L./pa2 -lruntime))

.PHONY: clean
clean:
	rm -rf $(BINDIR)
	rm -rf $(TARDIR)
