#include "HTTPClient.hpp"

// Store local callbacks in an unnamed namespace
namespace {
// Data callback for the received data
size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                     std::string *received_data) {
  size_t new_length = size * nmemb;

  received_data->append(reinterpret_cast<char *>(contents), new_length);
  return new_length;
}
}  // namespace

std::string HTTPClient::PostJson(const std::string &uri,
                                 const std::string &json_data) {
  SPDLOG_INFO(
      "HTTPClient::PostJson : Making a GCloud API request to parse the "
      "speech.");

  SPDLOG_DEBUG("HTTPClient::PostJson : URI: {}, json_data size: {}", uri,
               json_data.size());

  // Setup CURL handles
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();

  // Response data buffer
  std::string received_data;
  if (curl) {
    // Setup CURL for an HTTPS POST request with JSON as body
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);

    curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_data.size());

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &received_data);

// Perform the request, unless in benchmark mode
#ifndef DETECTOR_BENCHMARK
    res = curl_easy_perform(curl);
#endif

    if (res != CURLE_OK) {
      std::string error(curl_easy_strerror(res));
      throw std::runtime_error("curl_easy_perform() failed: " + error);
    }

    // Cleanup
    delete headers;
    curl_easy_cleanup(curl);
  }

  SPDLOG_INFO(
      "HTTPClient::PostJson : Finished making a GCloud API request to parse "
      "the speech.");

  SPDLOG_DEBUG("HTTPClient::PostJson : Received data: {}.", received_data);

// Return a predetermined data in case of benchmark mode
#ifdef DETECTOR_BENCHMARK
  received_data =
      "{\"results\":[{\"alternatives\": [{\"transcript\": \"TEST TEST TEST "
      "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
      "TEST\"}]}]}";
#endif
  return received_data;
}
