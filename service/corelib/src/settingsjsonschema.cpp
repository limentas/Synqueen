
#include "settings.hpp"

#include <string>

namespace synqueen {

const std::string SettingsProvider::jsonSchema = R"(
{
  "$schema": "https://json-schema.org/draft-04/schema",
  "type": "object",
  "required": ["version"],
  "properties": {
    "version": {
      "type": "integer"
    },
    "folders": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["path", "cloudSyncPoints"],
        "properties": {
          "path": {
            "type": "string"
          },
          "cloudSyncPoints": {
            "type": "array",
            "items": {
              "type": "object",
              "properties": {
                "driver": {
                  "type": "string"
                }
              },
              "required": ["driver"]
            }
          }
        }
      }
    }
  }
}
)";

} // namespace synqueen
