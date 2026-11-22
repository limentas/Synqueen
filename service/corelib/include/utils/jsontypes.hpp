#pragma once

#include <rapidjson/document.h>

#include <memory>

namespace json = rapidjson;

namespace synqueen::utils {

typedef std::shared_ptr<json::GenericDocument<json::UTF8<>>>
    JsonDocumentDefaultPtr;
typedef std::shared_ptr<json::GenericValue<json::UTF8<>, json::CrtAllocator>>
    JsonValueCrtPtr;

} // namespace synqueen::utils
