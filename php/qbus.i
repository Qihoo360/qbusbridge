%module(directors="1") qbus
%include <stl.i>

#define NOT_USE_CONSUMER_CALLBACK

%{
#include "../cxx/src/qbus_producer.h"
#include "../cxx/src/qbus_consumer.h"
#include "../cxx/src/qbus_consumer_callback.h"
%}

%include "std_string.i"
%include "std_vector.i"

namespace std {
  %template(StringVector) vector<std::string>;
}

%feature("director") QbusMsgContentInfo;

%include "../cxx/src/qbus_producer.h"
%include "../cxx/src/qbus_consumer.h"
%include "../cxx/src/qbus_consumer_callback.h"

