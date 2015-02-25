#ifndef HOMEBREW_BROWSER_HTTPGETSERVICE_H_
#define HOMEBREW_BROWSER_HTTPGETSERVICE_H_

#include <functional>
#include <string>

#include <3ds.h>

extern u32 const kUnknownRequestSize;
extern u32 const kUnknownHttpStatus;

enum class HttpGetRequestError {
  kNone,
  kCouldntAllocateContext,
  kCouldntBeginReqest,
  kBadStatusCode,
  kBadRequestState,
  kBadDownloadSize,
  kReceiveDataFailed,
};

struct HttpGetRequestState {
  std::string url{""};
  
  // State of the current transfer
  bool finished{false};
  HttpGetRequestError error{HttpGetRequestError::kNone};

  // Progress
  struct {
    u32 expected_size{0};
    u32 current_size{0};
  } response;

  // Output! (note: if not defined, output is silently discarded!)
  std::function<void(u32, u8 const*)> on_data{nullptr};

  // Internal state of the http request
  httpcContext context;
  u32 status_code_{kUnknownHttpStatus};
};

HttpGetRequestState InitiateRequest(std::string const& url,
    std::function<void(u32, u8 const*)> const on_data,
    u32 const expected_size = kUnknownRequestSize);
HttpGetRequestState ProcessRequest(HttpGetRequestState const& state);
HttpGetRequestState CancelRequest(HttpGetRequestState const& state);

#endif
