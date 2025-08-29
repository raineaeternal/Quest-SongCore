#pragma once

#include "beatsaber-hook/shared/rapidjson/include/rapidjson/document.h"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/error/en.h"
#include "logging.hpp"
#include "paper2_scotland2/shared/utfcpp/source/utf8.h"
#include <filesystem>
#include <optional>
#include <string>

namespace SongCore::Utils {
template <typename T, typename S = std::string_view>
void PrintJSONError(const rapidjson::GenericDocument<T> &doc,
                    const std::string_view &context, const S &data) {
  if (!doc.HasParseError())
    return;

  auto error = rapidjson::GetParseError_En(doc.GetParseError());
  ERROR("Failed to parse JSON {}! Error: {} at: {}", context, error,
        doc.GetErrorOffset());

  if (data.empty()) {
    ERROR("JSON source string is empty.");
    return;
  }

  size_t errorOffset = static_cast<size_t>(doc.GetErrorOffset());

  if (data.size() < errorOffset) {
    ERROR("Raw JSON data is smaller than error offset!");
    return;
  }

  size_t errorRadius = 400;

  size_t start = (errorOffset > errorRadius) ? (errorOffset - errorRadius)
                                             : 0;
  size_t end =
      std::min(errorOffset + errorRadius, data.size());

  S beforeError = data.substr(start, errorOffset - start);
  S afterError = data.substr(errorOffset, end - errorOffset);

  // If the type is utf16 then convert it, otherwise just use it as is
  if constexpr (std::is_same_v<S, std::u16string_view>) {
    ERROR("...{}-->{}...", utf8::utf16to8(beforeError),
          utf8::utf16to8(afterError));
  } else {
    ERROR("...{}-->{}...", beforeError, afterError);
  }
}
} // namespace SongCore::Utils
