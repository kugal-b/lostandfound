#ifndef ITEM_H
#define ITEM_H

#include <string>

using namespace std;

struct Item {
    int id;
    string name;
    string category;
    string color;
    string location;
    string date;
    string description;
    string status;
};

#endif