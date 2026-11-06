NAME := com.local.disablesleep-by-power-source
BUILD_DIR := build
BINARY := $(BUILD_DIR)/$(NAME)
SOURCE := src/disablesleep_by_power_source.c
PLIST := launchd/$(NAME).plist
SCRIPTS := scripts/install.sh scripts/status.sh scripts/uninstall.sh

CC ?= clang
CFLAGS ?= -Wall -Wextra -Werror -O2
FRAMEWORKS := -framework CoreFoundation -framework IOKit

.PHONY: all build check lint clean install uninstall status

all: build

build: $(BINARY)

$(BINARY): $(SOURCE)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(FRAMEWORKS) $(SOURCE) -o $(BINARY)

lint:
	plutil -lint $(PLIST)
	zsh -n $(SCRIPTS)

check: lint build
	$(BINARY) --dry-run
	$(BINARY) --test-assertions

install:
	./scripts/install.sh

uninstall:
	./scripts/uninstall.sh

status:
	./scripts/status.sh

clean:
	rm -f $(BINARY)
	rmdir $(BUILD_DIR) 2>/dev/null || true
