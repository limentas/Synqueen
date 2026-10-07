#include "utils/utils.hpp"

#include <cassert>
#include <cctype>
#include <cstdio>
#include <ranges>
#include <string>

using namespace std;

namespace fs = std::filesystem;

namespace synqueen::utils {

string toPrintable(const char *data, size_t length) {
  string output;
  output.reserve(length * 4); // Worst case: all characters are unprintable
  for (size_t i = 0; i < length; ++i) {
    auto c = data[i];
    if (isprint(static_cast<unsigned char>(c))) {
      output += c;
    } else {
      char buffer[5];
      snprintf(buffer, sizeof(buffer), "\\x%02X",
               static_cast<unsigned char>(c));
      output += buffer;
    }
  }
  return output;
}

string toPrintable(const string &input) {
  return toPrintable(input.data(), input.size());
}

string replaceAll(const char *input, const char *search, const char *replace) {
  return replaceAll(string_view(input), string_view(search),
                    string_view(replace));
}

string replaceAll(const string_view &input, const string_view &search,
                  const string_view &replace) {
  assert(!search.empty() && "Search string must not be empty");
  return input | views::split(search) | views::join_with(replace) |
         ranges::to<string>();
}

string replaceAll(const char *input,
                  list<pair<const char *, const char *>> replacements) {
  return replaceAll(
      string_view(input),
      list<pair<string, string>>(replacements.begin(), replacements.end()));
}

string replaceAll(const string_view &input,
                  list<pair<string, string>> replacements) {
  string result = string(input);
  for (const auto &replacement : replacements) {
    result = replaceAll(result, replacement.first, replacement.second);
  }
  return result;
}

std::string removeAtEnd(const std::string &input,
                        const std::string &characters) {
  std::string result = input;
  while (!result.empty() &&
         characters.find(result.back()) != std::string::npos) {
    result.pop_back();
  }
  return result;
}

corral::Task<std::string> createTemporaryFolder(const std::string &templateStr,
                                                uv_loop_t *loop) {
  auto tmpDir = fs::temp_directory_path();
  uv_fs_t req;
  using CBPType = corral::CBPortal<uv_fs_t *>;
  CBPType cbp;
  auto tempRepoTemplate = (tmpDir / templateStr).string();
  auto r = co_await corral::untilCBCalled(
      [&](CBPType::Callback &cb) {
        req.data = &cb;
        uv_fs_mkdtemp(
            loop, &req, tempRepoTemplate.c_str(),
            +[](uv_fs_t *r) { (*(CBPType::Callback *)r->data)(r); });
      },
      cbp);
  if (r->result < 0) {
    throw std::runtime_error(
        "Failed to create temporary folder for template: " + tempRepoTemplate);
  }
  auto tempRepoPath = std::string(r->path);
  uv_fs_req_cleanup(&req);
  co_return tempRepoPath;
}

corral::Task<std::string> createTemporaryFile(const std::string &templateStr,
                                              uv_loop_t *loop) {
  auto tmpDir = fs::temp_directory_path();
  uv_fs_t req;
  using CBPType = corral::CBPortal<uv_fs_t *>;
  CBPType cbp;
  auto tempRepoTemplate = (tmpDir / templateStr).string();
  auto r = co_await corral::untilCBCalled(
      [&](CBPType::Callback &cb) {
        req.data = &cb;
        uv_fs_mkstemp(
            loop, &req, tempRepoTemplate.c_str(),
            +[](uv_fs_t *r) { (*(CBPType::Callback *)r->data)(r); });
      },
      cbp);
  if (r->result < 0) {
    throw std::runtime_error("Failed to create temporary file for template: " +
                             tempRepoTemplate);
  }
  auto tempRepoPath = std::string(r->path);

  // Close the temporary file after creation
  co_await corral::untilCBCalled(
      [&](CBPType::Callback &cb) {
        req.data = &cb;
        uv_fs_close(
            loop, &req, r->result,
            +[](uv_fs_t *r) { (*(CBPType::Callback *)r->data)(r); });
      },
      cbp);
  uv_fs_req_cleanup(&req);
  co_return tempRepoPath;
}

} // namespace synqueen::utils
