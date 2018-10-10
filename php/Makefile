TOP        = ./
SWIGEXE    = /usr/local/bin/swig
SWIG_LIB_DIR = /usr/local/share/swig/3.0.12/
CXXSRCS    = ../cxx/src/qbus_record_msg.cc ../cxx/src/qbus_consumer.cc ../cxx/src/qbus_producer.cc ../cxx/src/qbus_config.cc ../cxx/src/qbus_helper.cc ../cxx/util/logger.cc ../cxx/src/qbus_consumer_imp.cc
TARGET     = qbus
INTERFACE  = qbus.i

check: build
	$(MAKE) -f $(TOP)/TopMakefile SRCDIR='$(SRCDIR)' php_run

build:
	$(MAKE) -f $(TOP)/TopMakefile SRCDIR='$(SRCDIR)' $(SWIGLIB) CXXSRCS='$(CXXSRCS)' \
	SWIG_LIB_DIR='$(SWIG_LIB_DIR)' SWIGEXE='$(SWIGEXE)' \
	SWIGOPT='$(SWIGOPT)' TARGET='$(TARGET)' INTERFACE='$(INTERFACE)' php_cpp #\
	PHP_SRC='$(PHP_SRC)' \
	PHP_VER='$(PHP_VER)' \
	CENT_VER='$(CENT_VER)'

clean:
	$(MAKE) -f $(TOP)/TopMakefile SRCDIR='$(SRCDIR)' php_clean
