#ifndef ITEM_H
#define ITEM_H

#include <string>

struct Item
{
    int id = 0;
    int reporterId = 0;      // user who reported this item (owner / finder)
    std::string name;
    std::string category;
    std::string color;
    std::string location;
    std::string date;        // YYYY-MM-DD
    std::string description;
    std::string status;      // LOST, FOUND, RECOVERED, RETURNED
};

#endif