#pragma once

#include "utils/jsontypes.hpp"

#include <rapidjson/document.h>

namespace RAPIDJSON_NAMESPACE {

::std::ostream &operator<<(::std::ostream &os, const rapidjson::Document &doc);
::std::ostream &operator<<(::std::ostream &os, const rapidjson::Value &value);
::std::ostream &operator<<(::std::ostream &os,
                           const GenericValue<UTF8<>, CrtAllocator> &value);

} // namespace RAPIDJSON_NAMESPACE
