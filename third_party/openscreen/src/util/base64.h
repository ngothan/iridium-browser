// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_BASE64_H_
#define UTIL_BASE64_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "platform/base/error.h"
#include "platform/base/span.h"

namespace openscreen {
namespace base64 {

// Encodes the input binary data in base64.
std::string Encode(ByteView input);

// Encodes the input string in base64.
std::string Encode(absl::string_view input);

// Decodes the base64 input string.  Returns true if successful and false
// otherwise. The output string is only modified if successful. The decoding can
// be done in-place.
bool Decode(absl::string_view input, std::vector<uint8_t>* output);

}  // namespace base64
}  // namespace openscreen

#endif  // UTIL_BASE64_H_
