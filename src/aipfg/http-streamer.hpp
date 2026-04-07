#ifndef HTTP_STREAMER_HPP
#define HTTP_STREAMER_HPP

#include <cstdint>
#include <map>
#include <string>

// Performs a streaming HTTP POST. As bytes arrive from the server they are
// delivered in-order to on_chunk; libcurl is never visible to the caller.
class HttpStreamer
{
public:
  HttpStreamer();
  ~HttpStreamer();

  HttpStreamer(const HttpStreamer&) = delete;
  HttpStreamer& operator=(const HttpStreamer&) = delete;

  template<typename F>
  void post(const std::string& url, const std::string& body,
            const std::map<std::string, std::string>& headers, F&& on_chunk)
  {
    post_impl(url, body, headers,
              [](const uint8_t* data, size_t n, void* ud) {
                (*static_cast<F*>(ud))(data, n);
              },
              &on_chunk);
  }

private:
  void post_impl(const std::string& url, const std::string& body,
                 const std::map<std::string, std::string>& headers,
                 void (*callback)(const uint8_t*, size_t, void*),
                 void* userdata);

  void* curl_handle_;
};

#endif // HTTP_STREAMER_HPP
