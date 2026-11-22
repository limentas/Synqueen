#include "gtestprinters.hpp"

#include <ostream>
#include <rapidjson/writer.h>

namespace RAPIDJSON_NAMESPACE {

::std::ostream &operator<<(::std::ostream &os, const Document &doc) {
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  doc.Accept(writer);
  os << buffer.GetString();
  return os;
}

::std::ostream &operator<<(::std::ostream &os, const Value &value) {
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  value.Accept(writer);
  os << buffer.GetString();
  return os;
}

::std::ostream &operator<<(::std::ostream &os,
                           const GenericValue<UTF8<>, CrtAllocator> &value) {
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  value.Accept(writer);
  os << buffer.GetString();
  return os;
}

} // namespace RAPIDJSON_NAMESPACE
