CXX = /usr/bin/g++
CXX_FLAGS = -g
INC_DIR = ../../src
LIB_DIR = ../../lib/debug

TARGETS = qbus_consumer_example \
		  qbus_pause_resume_example

all: $(TARGETS)
	@echo "Build targets: "$(TARGETS)

%: %.cc
	$(CXX) -o $@ $< $(CXX_FLAGS) -I $(INC_DIR) -L $(LIB_DIR) -lQBus -Wl,-rpath=$(LIB_DIR)

.Phony:
clean:
	rm -f $(TARGETS)
