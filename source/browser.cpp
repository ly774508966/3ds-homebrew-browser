#include "browser.h"

#include "http.h"
#include "storage.h"
#include "smdh.h"

#include "debug.h"
#include "util.h"

#include <map>
#include <algorithm>

using std::string;

Result download_app(std::string const& server, std::string const& title) {
  string appname = title.substr(title.find("/") + 1);
  debug_message("Downloading " + title + " from " + server + "...", true);
  Result error{0};
  std::vector<std::string> title_file_listing;
  std::tie(error, title_file_listing) =
      get_file_listing_for_title(server, title);

  for (auto const& relative_path : title_file_listing) {
    string server_path = "/3ds/" + title + "/" + relative_path;
    debug_message("Downloading file: " + server_path, true);
    std::vector<u8> file_contents;
    std::tie(error, file_contents) = http_get(server + server_path);
    debug_message("Writing file: /3ds/" + appname + "/" + relative_path);
    write_file("/3ds/" + appname + "/" + relative_path, 
        &file_contents[0], file_contents.size());
  }
  debug_message("Finished.");
  return error;
}

Result download_smdh(std::string const& server, Title const& title, AppInfo& app_info) {
  Result error{0};
  std::vector<u8> smdh_byte_buffer;
  std::tie(error, smdh_byte_buffer) = http_get(server + "/" + title.path + "/smdh");
  if (error) {
    return error;
  }

  char smdh_title[0x40];
  char smdh_author[0x40];
  char smdh_description[0x80];
  smdh_s* const smdh = reinterpret_cast<smdh_s*>(&smdh_byte_buffer[0]);

  extractSmdhData(smdh, smdh_title, smdh_description, smdh_author, &(*app_info.image)[0]);
  app_info.title = smdh_title;
  app_info.author = smdh_author;
  app_info.description = smdh_description;

  return error;
}

void download_smdh_for_page(std::string const& server,
    TitleListCursor const& cursor, std::array<AppInfo, 3>& smdh_cache) {
  bool const there_are_no_titles = cursor.begin == cursor.end;
  if (there_are_no_titles) {
    std::for_each(begin(smdh_cache), end(smdh_cache), [](AppInfo& info) {
      info.title = "no title";
      info.author = "";
      info.description = "";
    });
    return;
  }

  auto const visible_titles_begin = std::next(cursor.begin,
      std::distance(cursor.begin, cursor.selected) / 3 * 3);
  auto const visible_titles_end =
      std::distance(visible_titles_begin, cursor.end) < 3 ?
      cursor.end : std::next(visible_titles_begin, 3);

  // std::transform(visible_titles_begin, visible_titles_end, begin(smdh_cache),
  //     [&server](Title const& title) {
  auto app_info = begin(smdh_cache);
  auto visible_title = visible_titles_begin;
  for (; app_info != end(smdh_cache) and visible_title != visible_titles_end;
      ++app_info, ++visible_title) {
    // This assumes that there is always an icon. This wont always be the case,
    // so this needs to be adjusted to place a default icon in the slot and
    // empty out the strings on a failed download.
    Result error = download_smdh(server, *visible_title, *app_info);
    if (error) {
      debug_message("SMDH Download ERR: " + string_from<int>(error));
    }
  }
  // });
}

void switch_to_category(SelectedCategory category, BrowserState& state) {
  Result error{0};
  std::tie(error, state.homebrew_listing) = get_homebrew_listing(kServer, category);
  sort_homebrew_list(state);
  state.selected_index = 0;
  sort_homebrew_list(state);
}

TitleListCursor get_title_list_cursor(TitleList const& titles,
    TitleList::size_type const& offset) {
  return TitleListCursor{
    begin(titles),
    end(titles),
    std::next(begin(titles), offset)
  };
}

std::map<SelectedCategory, string> g_category_names {
  {SelectedCategory::kGames, "games"},
  {SelectedCategory::kMedia, "media"},
  {SelectedCategory::kEmulators, "emulators"},
  {SelectedCategory::kTools, "tools"},
  {SelectedCategory::kMisc, "misc"}
};

std::tuple<Result, std::vector<Title>> get_homebrew_listing(std::string const& server_url, SelectedCategory category) {
  Result error;
  std::vector<std::string> raw_listing;
  if (category == SelectedCategory::kNone) {
    std::tie(error, raw_listing) = download_and_split_on_newlines(server_url + "/homebrew_list");
  } else {
    std::tie(error, raw_listing) = download_and_split_on_newlines(server_url + "/" + g_category_names[category] + "/homebrew_list");
  }
  std::vector<Title> title_list;
  for (auto path : raw_listing) {
    title_list.push_back({
      path,
      path.substr(path.find("/") + 1),
      path.substr(0, path.find("/")),
      server_url
    });
  }
  return std::make_tuple(error, title_list);
}

void sort_homebrew_list(BrowserState& state) {
  std::sort(begin(state.homebrew_listing), end(state.homebrew_listing));
  if (state.sort_order == ListingSortOrder::kAlphanumericDescending) {
    std::reverse(begin(state.homebrew_listing), end(state.homebrew_listing));
  }
  download_smdh_for_page(kServer, get_title_list_cursor(state.homebrew_listing,
      state.selected_index), state.app_info_for_current_page);
}