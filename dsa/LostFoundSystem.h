#ifndef LOSTFOUNDSYSTEM_H
#define LOSTFOUNDSYSTEM_H

#include "Item.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>

// Claim structure
struct Claim
{
    int claimId = 0;
    int lostItemId = 0;
    int foundItemId = 0;
    int claimantId = 0;
    int finderId = 0;
    std::string status;      // PENDING, ACCEPTED, REJECTED, CANCELLED
};

// Matching structure
struct MatchResult
{
    int lostItemId;
    int foundItemId;
    int score;
};

// Highest score gets highest priority; ties go to the lower found-item ID
struct CompareMatch
{
    bool operator()(const MatchResult& a, const MatchResult& b) const
    {
        if (a.score != b.score)
            return a.score < b.score;
        return a.foundItemId > b.foundItemId;
    }
};

class LostFoundSystem
{
private:
    // Main storage
    std::vector<Item> lostItems;
    std::vector<Item> foundItems;
    std::vector<Claim> claims;

    // ID-based indexes (kept in sync with the vectors)
    std::unordered_map<int, Item> lostItemMap;
    std::unordered_map<int, Item> foundItemMap;

    // Duplicate checking (only for ACTIVE items)
    std::unordered_set<std::string> lostItemKeys;
    std::unordered_set<std::string> foundItemKeys;

    // ID counters (per system, not shared static state)
    int nextItemId = 1;
    int nextClaimId = 1;

    // Helpers
    std::string createItemKey(const Item& item) const;
    bool validateItem(Item& item);

public:
    // Item management
    void addLostItem(Item item);
    void addFoundItem(Item item);
    void displayLostItems();
    void displayFoundItems();
    bool deleteLostItem(int id);
    bool deleteFoundItem(int id);

    // Searching
    Item* searchById(int id);
    void searchByName(std::string name);
    void searchByCategory(std::string category);
    void searchByLocation(std::string location);

    // Matching
    int compareName(const Item& lost, const Item& found) const;
    int compareCategory(const Item& lost, const Item& found) const;
    int compareColor(const Item& lost, const Item& found) const;
    int compareLocation(const Item& lost, const Item& found) const;
    int compareDate(const Item& lost, const Item& found) const;
    int calculateMatchScore(const Item& lost, const Item& found) const;
    void findMatches();
    void displayBestMatches(
        std::priority_queue<MatchResult,
                            std::vector<MatchResult>,
                            CompareMatch>& matches);

    // Claims
    void createClaim(int lostItemId, int foundItemId, int claimantId, int finderId);
    void displayClaims(int finderId);
    void acceptClaim(int claimId, int finderId);
    void rejectClaim(int claimId, int finderId);
    void markAsReturned(int foundItemId);
    void markAsRecovered(int lostItemId);

    // Utility functions
    int generateId();
    int generateClaimId();
    bool checkDuplicate(const Item& item, bool isLost) const;
    void sortItems(std::vector<Item>& items, std::string criterion);
};

#endif