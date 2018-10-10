TOP        = ./
SWIGEXE    = /usr/local/bin/swig 
SWIG_LIB_DIR = /usr/local/share/swig/3.0.12
CXXSRCS    = ../cxx/src/qbus_record_msg.cc ../cxx/src/qbus_consumer.cc ../cxx/src/qbus_producer.cc ../cxx/src/qbus_config.cc ../cxx/src/qbus_helper.cc ../cxx/util/logger.cc
TARGET     = qbus
INTERFACE  = qbus.i
LIBS       = -Wl,-Bstatic -L../cxx/thirdparts/librdkafka/src -lrdkafka -Wl,-Bdynamic -lm -lzookeeper_mt 
SRCDIR = $(CURDIR)/

#check: build
#	$(MAKE) -f $(TOP)/TopMakefile SRCDIR='$(SRCDIR)' CXXSRCS='$(CXXSRCS)' TARGET='$(TARGET)' INTERFACE='$(INTERFACE)' go_run

build:
	$(MAKE) -f $(TOP)/TopMakefile SRCDIR='$(SRCDIR)' CXXSRCS='$(CXXSRCS)' \
	SWIG_LIB_DIR='$(SWIG_LIB_DIR)' SWIGEXE='$(SWIGEXE)' \
	TARGET='$(TARGET)' INTERFACE='$(INTERFACE)' go_cpp

clean:
	$(MAKE) -f $(TOP)/TopMakefile SRCDIR='$(SRCDIR)' INTERFACE='$(INTERFACE)' go_clean
