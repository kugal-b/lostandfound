#ifndef LOSTFOUNDSYSTEM_H
#define LOSTFOUNDSYSTEM_H
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <string>
#include "Item.h"

using namespace std;

class LostFoundSystem {

private:

    // Main storage
    vector<Item> lostItems;
    vector<Item> foundItems;

    // Fast ID lookup
    unordered_map<int, Item> itemMap;

    // Duplicate prevention
    unordered_set<string> itemKeys;

public:
    // 1. Item Management
    void addLostItem(Item item);
    void addFoundItem(Item item);

    void displayLostItems();
    void displayFoundItems();

    bool deleteLostItem(int id);
    bool deleteFoundItem(int id);

    // 2. Search
    Item* searchById(int id);
    void searchByName(string name);
    void searchByCategory(string category);
    void searchByLocation(string location);

    // 3. Matching
    int compareName(Item& lost, Item& found);
    int compareCategory(Item& lost, Item& found);
    int compareColor(Item& lost, Item& found);
    int compareLocation(Item& lost, Item& found);
    int compareDate(Item& lost, Item& found);

    int calculateMatchScore(Item& lost, Item& found);
    void findMatches(int lostItemId);
    void displayBestMatches(int lostItemId);

    // 4. Claims
    void createClaim(int lostItemId, int foundItemId, int claimantId);
    void displayClaims(int finderId);

    bool acceptClaim(int claimId, int finderId);
    bool rejectClaim(int claimId, int finderId);

    void markAsReturned(int foundItemId);

    // 5. Utility
    int generateId();
    bool checkDuplicate(Item& item);
    void sortItems();
};

#endif