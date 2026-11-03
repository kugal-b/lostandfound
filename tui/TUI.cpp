#include "TUI.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>
#include <memory>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#include <functional>
#include <stdexcept>

using namespace ftxui;
using namespace std;

namespace
{
  // ---------------------------------------------------------------------------
  // Theme
  // ---------------------------------------------------------------------------
  // TokyoNight (night) - the default LazyVim colorscheme.
  const Color kBg      = Color::RGB(0x1a, 0x1b, 0x26);  // bg
  const Color kAccent  = Color::RGB(0x7a, 0xa2, 0xf7);  // blue
  const Color kAccent2 = Color::RGB(0xbb, 0x9a, 0xf7);  // magenta
  const Color kCyan    = Color::RGB(0x7d, 0xcf, 0xff);  // cyan
  const Color kGood    = Color::RGB(0x9e, 0xce, 0x6a);  // green
  const Color kWarn    = Color::RGB(0xe0, 0xaf, 0x68);  // yellow
  const Color kBad     = Color::RGB(0xf7, 0x76, 0x8e);  // red
  const Color kMuted   = Color::RGB(0x73, 0x7a, 0xa2);  // dark5 / comment
  const Color kText    = Color::RGB(0xc0, 0xca, 0xf5);  // fg
  const Color kPanelBg = Color::RGB(0x29, 0x2e, 0x42);  // bg_highlight
  const Color kFocusBg = Color::RGB(0x28, 0x34, 0x57);  // visual selection
  const Color kLine    = Color::RGB(0x3b, 0x42, 0x61);  // fg_gutter (dividers)
  const Color kBorder  = Color::RGB(0x3d, 0x59, 0xa1);  // blue0 (float border)
  const Color kDark    = Color::RGB(0x15, 0x16, 0x1e);  // bg_dark (text on pills)

  // Capture output from the existing DSA display/search functions.
  // This lets the TUI reuse the current functions without changing them.
  string captureOutput(const function<void()>& action)
  {
    ostringstream output;
    streambuf* oldBuffer = cout.rdbuf(output.rdbuf());
    action();
    cout.rdbuf(oldBuffer);
    return output.str();
  }

  string itemDetails(const Item& item)
  {
    ostringstream output;
    output << "ID: " << item.id << '\n';
    output << "Name: " << item.name << '\n';
    output << "Category: " << item.category << '\n';
    output << "Color: " << item.color << '\n';
    output << "Location: " << item.location << '\n';
    output << "Date: " << item.date << '\n';
    output << "Description: " << item.description << '\n';
    output << "Status: " << item.status << '\n';
    return output.str();
  }

  // ---------------------------------------------------------------------------
  // Presentation helpers (no application logic lives here)
  // ---------------------------------------------------------------------------
  string toLower(string s)
  {
    for (char& c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return s;
  }

  vector<string> splitLines(const string& text)
  {
    vector<string> lines;
    string line;
    istringstream in(text);
    while (getline(in, line)) {
      if (!line.empty() && line.back() == '\r') line.pop_back();
      lines.push_back(line);
    }
    return lines;
  }

  bool isRuleLine(const string& line)
  {
    if (line.size() < 3) return false;
    for (char c : line)
      if (c != '-' && c != '=' && c != '*' && c != '_' && c != '~' && c != ' ') return false;
    return true;
  }

  Element statusBadge(const string& value)
  {
    string v = toLower(value);
    Color c = kAccent2;
    if (v.find("lost") != string::npos) c = kWarn;
    else if (v.find("found") != string::npos) c = kGood;
    else if (v.find("match") != string::npos || v.find("claim") != string::npos) c = kAccent;
    return text(" " + value + " ") | color(kDark) | bgcolor(c) | bold;
  }

  // One line of captured DSA output, styled. "Key: value" lines get an
  // aligned, dimmed key; rule lines become a thin divider.
  Element styledLine(const string& line)
  {
    if (line.empty()) return text("");
    if (isRuleLine(line)) return separatorLight() | color(kLine);

    size_t colon = line.find(':');
    if (colon != string::npos && colon > 0 && colon <= 13 && line[0] != ' ') {
      string key = line.substr(0, colon);
      string value = line.substr(colon + 1);
      size_t start = value.find_first_not_of(' ');
      value = (start == string::npos) ? "" : value.substr(start);

      string lowKey = toLower(key);
      Element valueEl;
      if (lowKey == "status") valueEl = hbox({ statusBadge(value), filler() });
      else if (lowKey == "name") valueEl = paragraph(value) | bold | color(kText) | bold;
      else if (lowKey == "id") valueEl = text(value) | bold | color(kAccent);
      else valueEl = paragraph(value) | color(kText);

      return hbox({
        text(key) | color(kMuted) | size(WIDTH, EQUAL, 13),
        text("│ ") | color(kMuted) | dim,
        valueEl | xflex
      });
    }
    return paragraph(line) | color(kText);
  }

  // Scrollable view for multi-line captured output. `cursor` is the line kept
  // in view; it is clamped here so callers never need to know the line count.
  Element scrollableText(const string& content, int& cursor)
  {
    vector<string> lines = splitLines(content);
    while (!lines.empty() && lines.back().empty()) lines.pop_back();
    if (lines.empty()) return text("");

    cursor = max(0, min(cursor, static_cast<int>(lines.size()) - 1));
    Elements rows;
    for (size_t i = 0; i < lines.size(); ++i) {
      Element row = styledLine(lines[i]);
      if (static_cast<int>(i) == cursor) row = row | focus;
      rows.push_back(row);
    }
    return vbox(std::move(rows)) | vscroll_indicator | yframe;
  }

  template <class T> string toStr(const T& v)
  {
    ostringstream o;
    o << v;
    return o.str();
  }

  // Items shown in a list. Built from real Item data (via searchById) so every
  // list renders as the same card, whatever format the DSA print functions use.
  struct ListView
  {
    vector<Item> items;
    string raw;            // captured DSA output (used as a fallback)
    bool structured = false;
    int sel = 0;
    int top = 0;           // first card drawn (windowed rendering)
    int version = -1;
    string key;
  };

  // Item IDs mentioned at the start of a line of captured output, in order.
  // Exact, and independent of which other fields the print function shows.
  vector<int> parseIds(const string& raw)
  {
    vector<int> ids;
    for (const string& line : splitLines(raw)) {
      string t = toLower(line);
      size_t i = t.find_first_not_of(" \t");
      if (i == string::npos) continue;
      if (t.compare(i, 7, "item id") == 0) i += 7;
      else if (t.compare(i, 2, "id") == 0) i += 2;
      else continue;
      while (i < t.size() && (t[i] == ' ' || t[i] == ':' || t[i] == '#' || t[i] == '=' || t[i] == '.' || t[i] == '-')) ++i;
      size_t start = i;
      while (i < t.size() && isdigit(static_cast<unsigned char>(t[i]))) ++i;
      if (i == start || i - start > 9) continue;
      if (i < t.size() && isalnum(static_cast<unsigned char>(t[i]))) continue;   // e.g. "id3x"
      int id = stoi(t.substr(start, i - start));
      if (find(ids.begin(), ids.end(), id) == ids.end()) ids.push_back(id);
    }
    return ids;
  }

  // Work out which items a captured DSA listing contains by matching the item
  // fields against the text, so this does not depend on the print format.
  // All items, fetched once per data change instead of on every keystroke.
  struct ItemCache
  {
    vector<Item> all;
    int version = -1;
  };

  void loadView(ListView& v, LostFoundSystem& sys, ItemCache& cache, int dataVersion,
                const string& raw, const string& statusHint)
  {
    v.raw = raw;
    v.items.clear();

    if (cache.version != dataVersion) {
      cache.all.clear();
      int misses = 0;
      for (int id = 1; id <= 5000 && misses < 300; ++id) {
        Item* p = nullptr;
        captureOutput([&] { p = sys.searchById(id); });
        if (!p) { ++misses; continue; }
        misses = 0;
        cache.all.push_back(*p);
      }
      cache.version = dataVersion;
    }
    const vector<Item>& all = cache.all;

    // Preferred: the listing prints "ID: n" for each item, so use those IDs.
    vector<int> ids = parseIds(raw);
    if (!ids.empty()) {
      for (int id : ids)
        for (const Item& it : all)
          if (toStr(it.id) == to_string(id)) { v.items.push_back(it); break; }
      if (!v.items.empty()) { v.structured = true; return; }
    }

    auto pick = [&](bool strict) {
      vector<pair<size_t, Item>> hits;
      for (const Item& it : all) {
        if (!statusHint.empty()) {
          string st = toLower(toStr(it.status));
          bool known = st.find("lost") != string::npos || st.find("found") != string::npos;
          if (known && st.find(statusHint) == string::npos) continue;
        }
        string name = toStr(it.name), loc = toStr(it.location),
               date = toStr(it.date), desc = toStr(it.description);
        size_t pos = raw.find(name);
        if (name.empty() || pos == string::npos) continue;
        if (!loc.empty() && raw.find(loc) == string::npos) continue;
        if (!date.empty() && raw.find(date) == string::npos) continue;
        if (strict && !desc.empty()) {
          size_t d = raw.find(desc);
          if (d == string::npos) continue;
          pos = d;
        }
        hits.push_back({pos, it});
      }
      stable_sort(hits.begin(), hits.end(),
                  [](const pair<size_t, Item>& a, const pair<size_t, Item>& b) { return a.first < b.first; });
      vector<Item> out;
      for (auto& h : hits) out.push_back(h.second);
      return out;
    };

    v.items = pick(true);
    if (v.items.empty()) v.items = pick(false);
    v.structured = !v.items.empty();
  }

  Element itemCard(const Item& it, bool selected)
  {
    auto kv = [&](const string& key, Element value) {
      return hbox({
        text(selected ? "▌ " : "  ") | color(kAccent) | bold,
        text(key) | color(kMuted) | size(WIDTH, EQUAL, 13),
        text("│ ") | color(kMuted) | dim,
        value | xflex
      });
    };
    Elements rows = {
      kv("ID", text(toStr(it.id)) | bold | color(kAccent)),
      kv("Name", paragraph(toStr(it.name)) | bold | color(kText) | bold),
      kv("Category", paragraph(toStr(it.category)) | color(kText)),
      kv("Color", paragraph(toStr(it.color)) | color(kText)),
      kv("Location", paragraph(toStr(it.location)) | color(kText)),
      kv("Date", paragraph(toStr(it.date)) | color(kText)),
      kv("Description", paragraph(toStr(it.description)) | color(kText)),
      kv("Status", hbox({ statusBadge(toStr(it.status)) }))   // no filler: a flexible row gets squashed first when space is tight
    };
    Element card = vbox(std::move(rows));
    if (selected) card = card | bgcolor(kPanelBg);
    return card;
  }

  // Three-line card used where vertical space is tight (search results).
  Element itemCardCompact(const Item& it, bool selected)
  {
    auto line = [&](Element content) {
      return hbox({ text(selected ? "▌ " : "  ") | color(kAccent) | bold, content | xflex });
    };
    Element card = vbox({
      line(hbox({
        text("#" + toStr(it.id) + "  ") | bold | color(kAccent),
        text(toStr(it.name)) | bold | color(kText),
        filler(),
        statusBadge(toStr(it.status))
      })),
      line(text(toStr(it.category) + "  ·  " + toStr(it.color) + "  ·  " +
                toStr(it.location) + "  ·  " + toStr(it.date)) | color(kCyan)),
      line(text(toStr(it.description)) | color(kMuted))
    });
    if (selected) card = card | bgcolor(kPanelBg);
    return card;
  }

  // Move the selection by `delta`, clamped to the list (works for fallback text too).
  void moveSel(ListView& v, int delta)
  {
    int count = v.structured ? static_cast<int>(v.items.size())
                             : static_cast<int>(count_if(v.raw.begin(), v.raw.end(), [](char c) { return c == '\n'; })) + 1;
    v.sel = max(0, min(v.sel + delta, max(0, count - 1)));
  }

  // The list: one card per item. Only the cards that fit on screen are built,
  // so scrolling stays instant however many items there are.
  Element renderItemList(ListView& v, bool showSelection, bool compact, int availRows)
  {
    if (!v.structured) {
      if (v.raw.empty()) return text("No items to show.") | color(kMuted) | dim;
      return scrollableText(v.raw, v.sel);
    }
    int count = static_cast<int>(v.items.size());
    v.sel = max(0, min(v.sel, count - 1));

    int cardHeight = compact ? 3 : 9;
    int visible = max(1, availRows / cardHeight);
    if (v.sel < v.top) v.top = v.sel;
    if (v.sel >= v.top + visible) v.top = v.sel - visible + 1;
    v.top = max(0, min(v.top, max(0, count - visible)));

    int end = min(count, v.top + visible + 1);   // +1 so a partly visible card fills the gap
    Elements cards;
    for (int i = v.top; i < end; ++i) {
      bool isSel = showSelection && i == v.sel;
      cards.push_back(compact ? itemCardCompact(v.items[i], isSel) : itemCard(v.items[i], isSel));
      if (!compact && i + 1 < end) cards.push_back(separatorLight() | color(kLine));
    }

    string info = " " + to_string(count) + (count == 1 ? " item" : " items") +
                  "   ·   " + to_string(v.sel + 1) + " / " + to_string(count);
    if (v.top > 0) info += "   ▲ " + to_string(v.top) + " above";
    if (v.top + visible < count) info += "   ▼ " + to_string(count - v.top - visible) + " below";
    return vbox({
      text(info) | color(kMuted),
      // yframe: cards keep their full height and the overflow is clipped, instead of
      // FTXUI squashing rows (e.g. the Status line) to make everything fit.
      vbox(std::move(cards)) | yframe | flex
    });
  }

  // A feedback message: red for problems, green otherwise.
  Element messageBanner(const string& message)
  {
    string trimmed = message;
    while (!trimmed.empty() && (trimmed.back() == '\n' || trimmed.back() == ' ')) trimmed.pop_back();
    if (trimmed.empty()) return text("");

    string low = toLower(trimmed);
    bool bad = low.find("not ") != string::npos || low.find("invalid") != string::npos ||
               low.find("valid numeric") != string::npos || low.find("error") != string::npos ||
               low.find("fail") != string::npos || low.find("cannot") != string::npos ||
               low.find("no item") != string::npos || low.find("empty") != string::npos;
    Color c = bad ? kBad : kGood;

    Elements rows;
    for (const string& line : splitLines(trimmed)) rows.push_back(paragraph(line));
    return hbox({
      text(bad ? " ✖ " : " ✔ ") | color(c) | bold,
      vbox(std::move(rows)) | color(c) | xflex
    }) | borderRounded | color(c);
  }

  Element pageHeader(const string& title, const string& subtitle)
  {
    return vbox({
      hbox({
        text("▌") | color(kAccent),
        text(" " + title) | bold | color(kText) | bold
      }),
      text("  " + subtitle) | color(kMuted),
      separatorLight() | color(kLine)
    });
  }

  Element formRow(const string& label, Component field)
  {
    return hbox({
      text(label) | color(kMuted) | size(WIDTH, EQUAL, 14) | vcenter,
      field->Render() | xflex
    });
  }

  Element keyHint(const string& key, const string& what)
  {
    return hbox({
      text(" " + key + " ") | bgcolor(kPanelBg) | color(kAccent) | bold,
      text(" " + what) | color(kMuted)
    });
  }

  Element infoCard(const string& icon, const string& title, const string& body, Color accent)
  {
    return vbox({
      hbox({ text(icon + " ") | color(accent) | bold, text(title) | bold | color(kText) | bold }),
      text(body) | color(kMuted)
    }) | borderRounded | color(accent) | size(WIDTH, GREATER_THAN, 32) | flex;
  }

  InputOption styledInput()
  {
    InputOption option;
    option.transform = [](InputState state) {
      state.element |= color(kText);
      if (state.is_placeholder) state.element |= color(kMuted) | dim;
      state.element = hbox({ text(" "), state.element, text(" ") });
      if (state.focused) state.element |= bgcolor(kFocusBg);
      else state.element |= bgcolor(kPanelBg);
      return state.element;
    };
    return option;
  }

  ButtonOption styledButton(Color accent)
  {
    ButtonOption option;
    option.transform = [accent](const EntryState& state) {
      Element e = text("  " + state.label + "  ") | bold;
      if (state.focused) e = e | color(kDark) | bgcolor(accent);
      else e = e | color(accent) | bgcolor(kPanelBg);
      return e;
    };
    return option;
  }

  void runTUIInternal(LostFoundSystem& system)
  {
    vector<string> navigation = {
      "Dashboard",
      "Browse Lost Items",
      "Browse Found Items",
      "Search Items",
      "Report an Item",
      "Delete an Item",
      "Exit"
    };
    const vector<string> navIcons = {"⌂", "◌", "●", "◎", "✚", "✖", "➜"};

    int selectedPage = 0;
    string outputMessage = "Welcome. Select an option from the sidebar.";

    // Result messages (item reported / deleted / errors) clear themselves after
    // a few seconds. 0 means "no expiry" (the initial welcome text stays).
    const long long kMessageMillis = 5000;
    auto nowMillis = [] {
      return static_cast<long long>(chrono::duration_cast<chrono::milliseconds>(
        chrono::steady_clock::now().time_since_epoch()).count());
    };
    atomic<long long> messageExpiry{0};
    auto armMessage = [&] { messageExpiry = nowMillis() + kMessageMillis; };
    auto expireMessage = [&] {
      long long at = messageExpiry;
      if (at != 0 && nowMillis() >= at) { outputMessage.clear(); messageExpiry = 0; }
    };

    // Scroll positions for long outputs (presentation only)
    // Item lists are rebuilt only when the data changes (dataVersion).
    int dataVersion = 0;
    ItemCache itemCache;
    auto lostView = make_shared<ListView>();
    auto foundView = make_shared<ListView>();
    auto searchView = make_shared<ListView>();

    // Search controls
    vector<string> searchTypes = {"Name", "Category", "Location", "ID"};
    int selectedSearchType = 0;
    string searchQuery;

    // Report form controls
    vector<string> reportTypes = {"Lost", "Found"};
    int selectedReportType = 0;
    string itemName, category, itemColor, location, date, description;

    // Delete controls
    vector<string> deleteTypes = {"Lost item", "Found item"};
    int selectedDeleteType = 0;
    string deleteId;

    // Sidebar menu
    MenuOption navOption = MenuOption::Vertical();
    navOption.entries_option.transform = [&](const EntryState& state) {
      // Look the entry up by label so this works across FTXUI versions.
      size_t index = static_cast<size_t>(
        find(navigation.begin(), navigation.end(), state.label) - navigation.begin());
      if (index >= navIcons.size()) index = 0;
      Element e = hbox({
        text(" " + navIcons[index] + "  "),
        text(state.label) | xflex
      });
      if (state.active) {
        if (state.focused) e = e | bold | color(kDark) | bgcolor(kAccent);
        else e = e | bold | color(kAccent) | bgcolor(kFocusBg);
      } else {
        e = e | color(index == 6 ? kBad : kText);
      }
      return e;
    };
    // FTXUI animates menu colour changes over 250 ms, redrawing the whole screen
    // dozens of times per keypress. Switch that off.
    navOption.entries_option.animated_colors.foreground.duration = std::chrono::milliseconds(0);
    navOption.entries_option.animated_colors.background.duration = std::chrono::milliseconds(0);
    navOption.underline.enabled = false;
    auto navMenu = Menu(&navigation, &selectedPage, navOption);

    // ---- Dashboard ----------------------------------------------------------
    auto dashboard = Renderer([&] {
      return vbox({
        pageHeader("Dashboard", "University Lost and Found System"),
        text("Welcome to the University Lost and Found System.") | color(kText) | bold,
        text("Use the sidebar to report, browse, search, or delete items.") | color(kMuted),
        text(""),
        text("AVAILABLE MODULES") | color(kAccent) | bold,
        hbox({
          infoCard("✚", "Item reporting", "Log lost or found items", kGood),
          text(" "),
          infoCard("●", "Browsing", "Lost and found item lists", kAccent2)
        }),
        hbox({
          infoCard("◎", "Search & filtering", "Name, category, location, ID", kWarn),
          text(" "),
          infoCard("✖", "Item deletion", "Remove an item by its ID", kBad)
        }),
        filler(),
        messageBanner(outputMessage)
      }) | flex | size(WIDTH, GREATER_THAN, 45) | xflex;
    });

    // ---- Browse pages -------------------------------------------------------
    // Browse pages display their items automatically when selected.
    // Up/Down select an item, PgUp/PgDn jump, the mouse wheel scrolls.
    auto makeBrowse = [&](const string& title, const string& subtitle,
                          shared_ptr<ListView> view, const string& hint,
                          function<void()> display) {
      auto page = Renderer([&, title, subtitle, view, hint, display](bool focused) {
        if (view->version != dataVersion) {
          loadView(*view, system, itemCache, dataVersion, captureOutput(display), hint);
          view->version = dataVersion;
        }
        return vbox({
          pageHeader(title, subtitle),
          renderItemList(*view, focused, false, max(3, Terminal::Size().dimy - 11)) | flex
        }) | flex;
      });
      return CatchEvent(page, [view](Event event) {
        if (event == Event::ArrowDown) { moveSel(*view, 1); return true; }
        if (event == Event::ArrowUp)   { moveSel(*view, -1); return true; }
        return false;
      });
    };
    auto lostRenderer = makeBrowse("Browse Lost Items", "Everything reported as lost",
                                   lostView, "lost", [&] { system.displayLostItems(); });
    auto foundRenderer = makeBrowse("Browse Found Items", "Everything reported as found",
                                    foundView, "found", [&] { system.displayFoundItems(); });

    // ---- Search -------------------------------------------------------------
    auto searchType = Dropdown(&searchTypes, &selectedSearchType);
    auto searchInput = Input(&searchQuery, "Type to search...", styledInput());

    // Results update automatically as the query or filter changes.
    auto refreshSearch = [&] {
      string key = to_string(selectedSearchType) + "|" + searchQuery + "|" + to_string(dataVersion);
      if (key == searchView->key) return;
      searchView->key = key;
      searchView->sel = 0;

      string results;
      if (selectedSearchType == 3 && searchQuery.empty()) {
        results = "Enter an ID to search.";
      } else {
        results = captureOutput([&] {
          if (selectedSearchType == 0) system.searchByName(searchQuery);
          else if (selectedSearchType == 1) system.searchByCategory(searchQuery);
          else if (selectedSearchType == 2) system.searchByLocation(searchQuery);
          else {
            try {
              size_t used = 0;
              int id = stoi(searchQuery, &used);
              if (used != searchQuery.size()) throw invalid_argument("Invalid ID");
              Item* item = system.searchById(id);
              if (item) cout << itemDetails(*item);
              else cout << "No item found with ID: " << id << '\n';
            } catch (...) {
              cout << "Please enter a valid numeric ID.\n";
            }
          }
        });
        if (results.empty()) results = "No matching items.";
      }
      // Drop the "Lost Item Found!" / "Found Item Found!" banner the search
      // functions print above each match; the result cards replace it.
      {
        string cleaned;
        for (const string& line : splitLines(results)) {
          string t = toLower(line);
          size_t first = t.find_first_not_of(" \t");
          size_t last = t.find_last_not_of(" \t");
          t = (first == string::npos) ? "" : t.substr(first, last - first + 1);
          if (t == "lost item found!" || t == "found item found!" ||
              t == "lost item found" || t == "found item found") continue;
          cleaned += line + "\n";
        }
        results = cleaned.empty() ? "No matching items." : cleaned;
      }
      loadView(*searchView, system, itemCache, dataVersion, results, "");
    };

    // Only the query box takes focus, so Up/Down can scroll the results directly.
    // Tab / Shift+Tab change the filter.
    auto searchPage = Container::Vertical({searchInput});
    auto searchRenderer = Renderer(searchPage, [&] {
      refreshSearch();

      Elements chips = { text("Filter  ") | color(kMuted) | size(WIDTH, EQUAL, 14) };
      for (int i = 0; i < static_cast<int>(searchTypes.size()); ++i) {
        Element chip = text(" " + searchTypes[i] + " ");
        chip = (i == selectedSearchType) ? chip | bold | color(kDark) | bgcolor(kAccent)
                                         : chip | color(kMuted) | bgcolor(kPanelBg);
        chips.push_back(chip);
        chips.push_back(text(" "));
      }
      chips.push_back(text(" Tab ") | color(kAccent) | bold);
      chips.push_back(text("to switch") | color(kMuted));

      string summary = searchView->structured
        ? "  " + to_string(searchView->items.size()) + (searchView->items.size() == 1 ? " match" : " matches")
        : "";
      Element body = searchView->structured
        ? renderItemList(*searchView, true, false, max(3, Terminal::Size().dimy - 11))
        : scrollableText(searchView->raw, searchView->sel);
      return vbox({
        // One-line header (the search controls need the vertical space for full cards).
        hbox({
          text("▌") | color(kAccent),
          text(" Search Items") | bold | color(kText),
          text("   updates as you type  ·  ↑↓ scroll") | color(kMuted)
        }),
        hbox(std::move(chips)),
        formRow("Query", searchInput),
        hbox({
          text("RESULTS") | bold | color(kAccent),
          text("  " + searchTypes[selectedSearchType] + (searchQuery.empty() ? "" : " ~ \"" + searchQuery + "\"")) | color(kMuted),
          text(summary) | color(kCyan)
        }),
        body | flex
      }) | flex;
    });
    searchRenderer |= CatchEvent([&](Event event) {
      int n = static_cast<int>(searchTypes.size());
      if (event == Event::Tab)        { selectedSearchType = (selectedSearchType + 1) % n; return true; }
      if (event == Event::TabReverse) { selectedSearchType = (selectedSearchType + n - 1) % n; return true; }
      if (event == Event::ArrowDown)  { moveSel(*searchView, 1); return true; }
      if (event == Event::ArrowUp)    { moveSel(*searchView, -1); return true; }
      return false;
    });

    // ---- Report -------------------------------------------------------------
    auto reportType = Dropdown(&reportTypes, &selectedReportType);
    auto nameInput = Input(&itemName, "Item name", styledInput());
    auto categoryInput = Input(&category, "Category", styledInput());
    auto colorInput = Input(&itemColor, "Color", styledInput());
    auto locationInput = Input(&location, "Location", styledInput());
    auto dateInput = Input(&date, "Date (DD-MM-YYYY)", styledInput());
    auto descriptionInput = Input(&description, "Description", styledInput());
    auto reportButton = Button("Submit item", [&] {
      Item item{};
      item.name = itemName;
      item.category = category;
      item.color = itemColor;
      item.location = location;
      item.date = date;
      item.description = description;

      outputMessage = captureOutput([&] {
        if (selectedReportType == 0) system.addLostItem(item);
        else system.addFoundItem(item);
      });

      ++dataVersion;
      armMessage();
      itemName.clear(); category.clear(); itemColor.clear();
      location.clear(); date.clear(); description.clear();
    }, styledButton(kGood));
    auto reportPage = Container::Vertical({reportType, nameInput, categoryInput,
      colorInput, locationInput, dateInput, descriptionInput, reportButton});
    auto reportRenderer = Renderer(reportPage, [&] {
      return vbox({
        pageHeader("Report an Item", "Fill in the details and submit"),
        formRow("Report type", reportType),
        text(""),
        formRow("Name", nameInput),
        formRow("Category", categoryInput),
        formRow("Color", colorInput),
        formRow("Location", locationInput),
        formRow("Date", dateInput),
        formRow("Description", descriptionInput),
        text(""),
        hbox({ text("") | size(WIDTH, EQUAL, 14), reportButton->Render() }),
        filler(),
        messageBanner(outputMessage)
      }) | flex;
    });

    // ---- Delete -------------------------------------------------------------
    auto deleteType = Dropdown(&deleteTypes, &selectedDeleteType);
    auto deleteInput = Input(&deleteId, "Enter item ID", styledInput());
    auto deleteButton = Button("Delete item", [&] {
      try {
        size_t used = 0;
        int id = stoi(deleteId, &used);
        if (used != deleteId.size()) throw invalid_argument("Invalid ID");
        bool deleted = selectedDeleteType == 0
          ? system.deleteLostItem(id)
          : system.deleteFoundItem(id);
        ++dataVersion;
        outputMessage = deleted ? "Item deleted." : "Item was not deleted.";
      } catch (...) {
        outputMessage = "Please enter a valid numeric ID.";
      }
      armMessage();
    }, styledButton(kBad));
    auto deletePage = Container::Vertical({deleteType, deleteInput, deleteButton});
    auto deleteRenderer = Renderer(deletePage, [&] {
      return vbox({
        pageHeader("Delete an Item", "Permanently remove an item by its ID"),
        formRow("Item type", deleteType),
        text(""),
        formRow("Item ID", deleteInput),
        text(""),
        hbox({ text("") | size(WIDTH, EQUAL, 14), deleteButton->Render() }),
        filler(),
        messageBanner(outputMessage)
      }) | flex;
    });

    // ---- Exit ---------------------------------------------------------------
    auto exitPage = Renderer([] {
      return vbox({
        pageHeader("Exit", "Close the application"),
        text(""),
        text("Press Escape or select another section to continue.") | color(kText),
        text("You can close the application with Escape.") | color(kMuted)
      }) | flex;
    });

    vector<Component> pages = {
      dashboard,
      lostRenderer,
      foundRenderer,
      searchRenderer,
      reportRenderer,
      deleteRenderer,
      exitPage
    };

    auto pageContainer = Container::Tab(pages, &selectedPage);
    // Two zones: 0 = sidebar, 1 = inside the selected page. Container::Tab only
    // delivers keys to the active zone, so nothing leaks between them.
    int zone = 0;
    auto root = Container::Tab({navMenu, pageContainer}, &zone);

    auto renderer = Renderer(root, [&] {
      expireMessage();
      Element sidebar = vbox({
        text(" NAVIGATION") | color(kMuted) | bold | dim,
        text(""),
        navMenu->Render() | flex
      }) | size(WIDTH, EQUAL, 26) | xflex_shrink;

      // lualine-style bars: coloured "mode" pill on the left.
      Element titleBar = hbox({
        text(" ◆ LOST & FOUND ") | bold | color(kDark) | bgcolor(kAccent),
        text(" "),
        text("University Lost and Found System") | color(kMuted),
        filler(),
        text(" " + navigation[selectedPage] + " ") | bold | color(kDark) | bgcolor(kAccent2)
      });

      Element footer = (zone == 0)
        ? hbox({
            text(" NAV ") | bold | color(kDark) | bgcolor(kAccent),
            text("  "),
            keyHint("↑↓ j k", "move"),
            text("  "),
            keyHint("→ l ⏎", "open page"),
            text("  "),
            keyHint("Esc", "quit"),
            filler()
          })
        : hbox({
            text(" PAGE ") | bold | color(kDark) | bgcolor(kGood),
            text("  "),
            keyHint("↑↓", "select"),
            text("  "),
            keyHint("PgUp/PgDn", "jump"),
            text("  "),
            keyHint("← h Esc", "back to sidebar"),
            filler()
          });

      return vbox({
        titleBar,
        separator() | color(kLine),
        hbox({
          sidebar,
          separator() | color(kLine),
          text(" "),
          pageContainer->Render() | flex,
          text(" ")
        }) | flex,
        separator() | color(kLine),
        footer
      }) | borderRounded | color(kBorder) | bgcolor(kBg) | flex;
    });

    auto screen = ScreenInteractive::Fullscreen();
    renderer |= CatchEvent([&](Event event) {
      if (event == Event::Custom) return false;   // timer wake-up, not user input
      if (event == Event::Escape) {
        if (zone == 1) { zone = 0; return true; }   // leave the page, back to the sidebar
        screen.Exit();                              // Esc in the sidebar quits
        return true;
      }
      if (selectedPage == 6) {
        screen.Exit();
        return true;
      }

      // Text field that currently has focus on the active page (nullptr if none).
      auto focusedText = [&]() -> string* {
        switch (selectedPage) {
          case 3: return searchInput->Focused() ? &searchQuery : nullptr;
          case 4:
            if (nameInput->Focused()) return &itemName;
            if (categoryInput->Focused()) return &category;
            if (colorInput->Focused()) return &itemColor;
            if (locationInput->Focused()) return &location;
            if (dateInput->Focused()) return &date;
            if (descriptionInput->Focused()) return &description;
            return nullptr;
          case 5: return deleteInput->Focused() ? &deleteId : nullptr;
          default: return nullptr;
        }
      };
      bool pageHasFocus = selectedPage >= 1 && selectedPage <= 5;

      if (event.is_mouse()) {
        // Clicking the sidebar / the page switches zone.
        if (event.mouse().button == Mouse::Left && event.mouse().motion == Mouse::Pressed) {
          if (event.mouse().x < 27) zone = 0;
          else if (pageHasFocus) zone = 1;
        }
      } else if (zone == 0) {
        // Sidebar: vim keys move, l / Right / Enter open the page.
        if (event == Event::Character('j')) { screen.PostEvent(Event::ArrowDown); return true; }
        if (event == Event::Character('k')) { screen.PostEvent(Event::ArrowUp); return true; }
        if (event == Event::ArrowRight || event == Event::Character('l') || event == Event::Return) {
          if (pageHasFocus) zone = 1;
          return true;
        }
      } else {
        // Inside a page: keys stay here. Only Left / h / Esc go back.
        string* field = focusedText();
        bool typing = field != nullptr;
        if (event == Event::ArrowLeft && (!typing || field->empty())) { zone = 0; return true; }
        if (!typing) {
          if (event == Event::Character('h')) { zone = 0; return true; }
          if (event == Event::Character('j')) { screen.PostEvent(Event::ArrowDown); return true; }
          if (event == Event::Character('k')) { screen.PostEvent(Event::ArrowUp); return true; }
        }
        if (event == Event::ArrowRight && !typing) return true;   // nothing to the right: stay put
      }

      // Page keys and touchpad / mouse-wheel scrolling for the item lists.
      shared_ptr<ListView> view;
      if (selectedPage == 1) view = lostView;
      else if (selectedPage == 2) view = foundView;
      else if (selectedPage == 3) view = searchView;
      if (view) {
        if (event == Event::PageDown) { moveSel(*view, 3); return true; }
        if (event == Event::PageUp)   { moveSel(*view, -3); return true; }
        if (selectedPage != 3 && event == Event::Home) { moveSel(*view, -1000000); return true; }
        if (selectedPage != 3 && event == Event::End)  { moveSel(*view, 1000000); return true; }
        if (event.is_mouse() && event.mouse().button == Mouse::WheelDown) { moveSel(*view, 1); return true; }
        if (event.is_mouse() && event.mouse().button == Mouse::WheelUp)   { moveSel(*view, -1); return true; }
      }
      return false;
    });

    atomic<bool> timerRunning{true};
    thread timer([&] {
      long long notified = 0;
      while (timerRunning) {
        this_thread::sleep_for(chrono::milliseconds(200));
        long long at = messageExpiry;
        if (at != 0 && at != notified && nowMillis() >= at) {
          notified = at;
          screen.PostEvent(Event::Custom);
        }
      }
    });
    screen.Loop(renderer);
    timerRunning = false;
    timer.join();
  }
}

void RunTUI(LostFoundSystem& system)
{
  // Keep the public entry point separate from the internal UI implementation.
  runTUIInternal(system);
}
