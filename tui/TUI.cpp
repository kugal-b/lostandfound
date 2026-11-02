
#include "TUI.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace std;
using namespace ftxui;

void RunTUI(LostFoundSystem& system)
{
    // Main menu
    vector<string> menuEntries = {
        "Search for an item",
        "Exit"
    };

    int selected = 0;
    int focus = 0;
    bool searchMode = false;

    // Search variables
    string query;
    vector<string> searchTypes = {
        "Name",
        "Category",
        "Location",
        "ID"
    };

    int searchTypeIndex = 0;

    string results = "Start typing to search for an item.";

    // Perform search automatically
    auto performSearch = [&]() {

        if (query.empty() && searchTypes[searchTypeIndex] == "ID") {
            results = "Start typing to search for an item.";
            return;
        }

        // Search by ID
        if (searchTypes[searchTypeIndex] == "ID") {

            try {
                size_t position;
                int id = stoi(query, &position);

                if (position != query.length()) {
                    results = "Please enter a valid numeric ID.";
                    return;
                }

                Item* item = system.searchById(id);

                if (item != nullptr) {
                    results = "\nITEM FOUND!\n\n";
                    results += "ID       : " + to_string(item->id) + "\n";
                    results += "Name     : " + item->name + "\n";
                    results += "Category : " + item->category + "\n";
                    results += "Color    : " + item->color + "\n";
                    results += "Location : " + item->location + "\n";
                    results += "Status   : " + item->status + "\n";
                }
                else {
                    results = "No item found with ID: " + query;
                }
            }
            catch (...) {
                results = "Please enter a valid numeric ID.";
            }

            return;
        }

        // Capture output from existing DSA search functions
        ostringstream output;
        streambuf* oldOutput = cout.rdbuf(output.rdbuf());

        if (searchTypes[searchTypeIndex] == "Name") {
            system.searchByName(query);
        }
        else if (searchTypes[searchTypeIndex] == "Category") {
            system.searchByCategory(query);
        }
        else if (searchTypes[searchTypeIndex] == "Location") {
            system.searchByLocation(query);
        }

        // Restore normal console output
        cout.rdbuf(oldOutput);

        results = output.str();

        if (results.empty()) {
            results = "No matching items found.";
        }
    };

    // Input options for live search
    InputOption inputOption;
    inputOption.on_change = performSearch;

    Component menu = Menu(&menuEntries, &selected);

    Component input = Input(
        &query,
        "Type here to search...",
        inputOption
    );

    // Container for navigation
    Component container = Container::Tab(
        {menu, input},
        &focus
    );

    auto screen = ScreenInteractive::Fullscreen();

    // Display interface
    Component renderer = Renderer(container, [&] {

        if (!searchMode) {

            return vbox({
                text("LOST & FOUND SYSTEM") | bold | center,
                text("UNIVERSITY ITEM RECOVERY") | center,

                separator(),

                text("MAIN MENU") | bold,
                menu->Render(),

                separator(),

                text("Use UP/DOWN arrows to navigate."),
                text("Press ENTER to select.")
            }) | border;
        }

        return vbox({
            text("LOST & FOUND - LIVE SEARCH") | bold | center,

            separator(),

            text("Search type: " +
                 searchTypes[searchTypeIndex]) | bold,

            input->Render() | border,

            text("Press TAB to change search type."),
            text("Results update automatically while typing."),

            separator(),

            text("SEARCH RESULTS") | bold,

            paragraph(results) | flex,

            separator(),

            text("ESC: Return to main menu")
        }) | border;
    });

    // Handle keyboard events
    Component app = CatchEvent(renderer, [&](Event event) {

        // Main menu
        if (!searchMode) {

            if (event == Event::Return) {

                if (selected == 0) {
                  searchMode = true;
                  focus = 1;

                  searchTypeIndex = 0;
                  query.clear();

                  performSearch();
                }
                else {
                    screen.ExitLoopClosure()();
                }

                return true;
            }
        }

        // Search screen
        else {

            if (event == Event::Escape) {
                searchMode = false;
                focus = 0;
                return true;
            }

            // Change search type
            if (event == Event::Tab) {

                searchTypeIndex =
                    (searchTypeIndex + 1) % searchTypes.size();

                performSearch();

                return true;
            }
        }

        return false;
    });

    // Start interface
    screen.Loop(app);
}

