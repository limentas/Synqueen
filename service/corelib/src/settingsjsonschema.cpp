
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
        "required": ["id", "path", "cloudDestinations"],
        "properties": {
          "id": {
            "type": "string"
          },
          "path": {
            "type": "string"
          },
          "cloudDestinations": {
            "type": "array",
            "items": {
              "type": "object",
              "properties": {
                "gateType": {
                  "type": "string"
                }
              },
              "required": ["gateType"]
            }
          }
        }
      }
    }
  }
}
)";

} // namespace synqueen
