#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Encode a byte buffer into a Base64 string
std::string Base64Encode(const std::vector<uint8_t>& data);

// Decode a Base64 string into a byte buffer
std::vector<uint8_t> Base64Decode(const std::string& base64);
