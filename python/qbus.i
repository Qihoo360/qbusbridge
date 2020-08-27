%module(directors="1") qbus
%include <stl.i>

%{
#include "../cxx/src/qbus_producer.h"
#include "../cxx/src/qbus_consumer.h"
%}

%include "std_string.i"
%include "std_vector.i"

namespace std {
  %template(StringVector) vector<std::string>;
}

%feature("director") QbusMsgContentInfo;
%feature("director") QbusConsumerCallback;

%include "../cxx/src/qbus_producer.h"
%include "../cxx/src/qbus_consumer.h"
