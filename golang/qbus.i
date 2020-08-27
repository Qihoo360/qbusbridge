%module(directors="1") qbus

%{
#include "../cxx/src/qbus_producer.h"
#include "../cxx/src/qbus_consumer.h"
%}

%include "std_string.i"
%include "std_vector.i"
%template(StringVector) std::vector<std::string>;

%feature("director") QbusMsgContentInfo;
%feature("director") QbusConsumerCallback;

%include "../cxx/src/qbus_producer.h"
%include "../cxx/src/qbus_consumer.h"

%insert(cgo_comment_typedefs) %{
#cgo LDFLAGS:-L${SRCDIR} -lQBus_go
%}
