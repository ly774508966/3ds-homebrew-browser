#ifndef HOMEBREW_BROWSER_HTTP_H_
#define HOMEBREW_BROWSER_HTTP_H_

#include <string>
#include <tuple>
#include <vector>
#include <functional>

#include <3ds.h>

#include "ui.h"

std::tuple<Result, std::vector<std::string>> get_file_listing_for_title(std::string const& server_url, std::string const& title);
std::tuple<Result, std::vector<std::string>> download_and_split_on_newlines(std::string const& url);

std::tuple<Result, std::vector<u8>> http_get(std::string const& url);
Result download_to_file(std::string const& url, std::string const& absolute_path, std::function<void (int)> report);

#endif  // HOMEBREW_BROWSER_HTTP_H_
