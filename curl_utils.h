#pragma once

#include <string>

// Perform a POST request with a JSON payload.
// Returns response as string. Throws std::runtime_error on failure.
std::string HttpPostJson(const std::string& url, const std::string& jsonPayload);

// Perform a GET request.
// Returns response as string. Throws std::runtime_error on failure.
std::string HttpGetJson(const std::string& url);

