#include "http-streamer.hpp"
#include <curl/curl.h>
#include <stdexcept>

namespace
{

struct CallbackData
{
  void (*callback)(const uint8_t*, size_t, void*);
  void* userdata;
};

size_t write_trampoline(char* ptr, size_t size, size_t nmemb, void* userdata)
{
  const size_t bytes = size * nmemb;
  auto* cbd = static_cast<CallbackData*>(userdata);
  cbd->callback(reinterpret_cast<const uint8_t*>(ptr), bytes, cbd->userdata);
  return bytes;
}

} // anonymous namespace

HttpStreamer::HttpStreamer()
{
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl_handle_ = curl_easy_init();
  if (!curl_handle_)
    throw std::runtime_error("Failed to initialize CURL");
}

HttpStreamer::~HttpStreamer()
{
  if (curl_handle_)
    curl_easy_cleanup(static_cast<CURL*>(curl_handle_));
  curl_global_cleanup();
}

void HttpStreamer::post_impl(const std::string& url, const std::string& body,
                             const std::map<std::string, std::string>& headers,
                             void (*callback)(const uint8_t*, size_t, void*),
                             void* userdata)
{
  CURL* curl = static_cast<CURL*>(curl_handle_);
  curl_easy_reset(curl);

  curl_slist* header_list = nullptr;
  for (const auto& [key, value] : headers)
    header_list = curl_slist_append(header_list, (key + ": " + value).c_str());

  CallbackData cbd{callback, userdata};

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_trampoline);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cbd);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

  const CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(header_list);

  if (res != CURLE_OK)
    throw std::runtime_error(std::string("HTTP request failed: ") +
                             curl_easy_strerror(res));
}
