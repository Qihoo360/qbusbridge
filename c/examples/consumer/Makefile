CC = /usr/bin/gcc
CC_FLAGS = -g -std=gnu99
INC_DIR = ../../src
LIB_DIR = ../../lib/debug

TARGETS = qbus_consumer_example

all: $(TARGETS)
	@echo "Build targets: "$(TARGETS)

%: %.c
	$(CC) -o $@ $< $(CC_FLAGS) -I $(INC_DIR) -L $(LIB_DIR) -lQBus_c -Wl,-rpath=$(LIB_DIR)

.Phony:
clean:
	rm -f $(TARGETS)
