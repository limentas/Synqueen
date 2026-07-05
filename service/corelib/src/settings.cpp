#include "settings.hpp"

#include <memory>
#include <rapidjson/document.h>
#include <rapidjson/encodedstream.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/schema.h>

#include <stdexcept>

using namespace rapidjson;

namespace synqueen {

Settings SettingsProvider::loadSettingsFromJson(const std::string &path) {
  auto fp = std::unique_ptr<FILE, decltype(&std::fclose)>(
      std::fopen(path.c_str(), "rb"), &std::fclose); // non-Windows use "r"
  if (!fp) {
    return createDefaultSettingsFile(path);
  }

  char readBuffer[1024];
  FileReadStream bis(fp.get(), readBuffer, sizeof(readBuffer));
  EncodedInputStream<UTF8<>, FileReadStream> eis(bis); // wraps bis into eis

  Document document;
  document.ParseStream<kParseNoFlags, UTF8<>>(eis);

  if (document.HasParseError()) {
    throw std::runtime_error("Failed to parse settings.json");
  }

  std::string errorMessage;
  if (!validateSchema(document, errorMessage)) {
    throw std::runtime_error(
        "Settings JSON does not conform to schema. Error: " + errorMessage);
  }

  auto version = document["version"].GetInt();
  if (version != myVersion) {
    throw std::runtime_error("Settings JSON version mismatch");
  }

  if (document.HasMember("folders")) {
    document["folders"].GetArray();
  }

  return Settings();
}

void SettingsProvider::saveSettingsToJson(const std::string &path,
                                          const Settings &settings) {
  auto fp = std::unique_ptr<FILE, decltype(&std::fclose)>(
      std::fopen(path.c_str(), "wb"), &std::fclose); // non-Windows use "w"
  if (!fp) {
    throw std::runtime_error("Failed to open settings.json for writing");
  }

  char writeBuffer[1024];
  FileWriteStream os(fp.get(), writeBuffer, sizeof(writeBuffer));
  typedef EncodedOutputStream<UTF8<>, FileWriteStream> OutputStream;
  OutputStream eos(os); // wraps os into eos
  PrettyWriter<OutputStream> writer(eos);
  writer.StartObject();
  writer.Key("version");
  writer.Int(myVersion);
  writer.Key("folders");
  writer.StartArray();
  for (const auto &folder : settings.folders) {
    writer.StartObject();
    writer.Key("path");
    writer.String(folder.path.c_str());
    writer.Key("cloudSyncPoints");
    writer.StartArray();
    for (const auto &cloudSyncPoint : folder.cloudSyncPoints) {
      writer.StartObject();
      writer.Key("driver");
      writer.String(cloudSyncPoint.driver.c_str());
      writer.EndObject();
    }
    writer.EndArray();
    writer.EndObject();
  }
  writer.EndArray();
  writer.EndObject();
}

Settings SettingsProvider::createDefaultSettingsFile(const std::string &path) {
  auto s = Settings();
  saveSettingsToJson(path, s);
  return s;
}

bool SettingsProvider::validateSchema(const Document &document,
                                      std::string &errorMessage) {
  SchemaDocument schema(Document().Parse(jsonSchema.c_str()));
  SchemaValidator validator(schema);

  if (!document.Accept(validator)) {
    // Standard way of getting errors lacks of details, so we output internal
    // json representation of the error.
    StringBuffer ss;
    typedef EncodedOutputStream<UTF8<>, StringBuffer> OutputStream;
    OutputStream eos(ss, false); // wraps ss into eos
    PrettyWriter<OutputStream> writer(eos);
    validator.GetError().Accept(writer);
    errorMessage = ss.GetString();
    return false;
  }

  return true;
}

} // namespace synqueen
