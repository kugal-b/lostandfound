#include "LostFoundSystem.h"
#include <iostream>
#include <algorithm>
#include <cctype>

using namespace std;

// Minimum score for two items to be considered a match
static const int MATCH_THRESHOLD = 60;

// STATIC HELPER

static string toLowerCase(string str)
{
    transform(str.begin(), str.end(), str.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(tolower(c));
        });
    return str;
}

static string trim(const string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Validates YYYY-MM-DD including month lengths and leap years
static bool isValidDate(const string& d)
{
    if (d.size() != 10 || d[4] != '-' || d[7] != '-')
        return false;

    for (size_t i = 0; i < d.size(); i++)
    {
        if (i == 4 || i == 7)
            continue;
        if (!isdigit(static_cast<unsigned char>(d[i])))
            return false;
    }

    int year  = stoi(d.substr(0, 4));
    int month = stoi(d.substr(5, 2));
    int day   = stoi(d.substr(8, 2));

    if (year < 1900 || month < 1 || month > 12 || day < 1)
        return false;

    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    if (leap)
        daysInMonth[1] = 29;

    return day <= daysInMonth[month - 1];
}

static void printItem(const Item& item)
{
    cout << "ID: " << item.id
         << "\nName: " << item.name
         << "\nCategory: " << item.category
         << "\nColor: " << item.color
         << "\nLocation: " << item.location
         << "\nDate: " << item.date
         << "\nDescription: " << item.description
         << "\nStatus: " << item.status
         << "\n-------------------------\n";
}

// Prints every active item in `items` that satisfies pred
template <typename Pred>
static bool printMatching(const vector<Item>& items,
                          const string& activeStatus,
                          const string& label,
                          Pred pred)
{
    bool any = false;
    for (const Item& item : items)
    {
        if (item.status == activeStatus && pred(item))
        {
            cout << "\n" << label << " Item Found!\n";
            printItem(item);
            any = true;
        }
    }
    return any;
}

// ITEM MANAGEMENT
int LostFoundSystem::generateId()
{
    return nextItemId++;
}

int LostFoundSystem::generateClaimId()
{
    return nextClaimId++;
}

// Key identifies "the same report": same item, place and day
string LostFoundSystem::createItemKey(const Item& item) const
{
    return toLowerCase(item.name) + "|" +
           toLowerCase(item.category) + "|" +
           toLowerCase(item.color) + "|" +
           toLowerCase(item.location) + "|" +
           item.date;
}

// Pure check: does NOT modify the key sets
bool LostFoundSystem::checkDuplicate(const Item& item, bool isLost) const
{
    string key = createItemKey(item);
    return isLost ? lostItemKeys.count(key) > 0
                  : foundItemKeys.count(key) > 0;
}

// Trims fields and validates them. Returns false (with a message) if invalid.
bool LostFoundSystem::validateItem(Item& item)
{
    item.name        = trim(item.name);
    item.category    = trim(item.category);
    item.color       = trim(item.color);
    item.location    = trim(item.location);
    item.date        = trim(item.date);
    item.description = trim(item.description);

    if (item.name.empty() || item.color.empty() ||
        item.category.empty() || item.location.empty() ||
        item.description.empty() || item.date.empty())
    {
        cout << "Please fill all the required details!\n";
        return false;
    }

    if (!isValidDate(item.date))
    {
        cout << "Invalid date! Use a real date in YYYY-MM-DD format.\n";
        return false;
    }

    if (item.reporterId <= 0)
    {
        cout << "Invalid user ID! It must be a positive number.\n";
        return false;
    }

    return true;
}

void LostFoundSystem::addLostItem(Item item)
{
    if (!validateItem(item))
        return;

    if (checkDuplicate(item, true))
    {
        cout << "Item already reported lost!\n";
        return;
    }

    item.id = generateId();
    item.status = "LOST";

    lostItemKeys.insert(createItemKey(item));
    lostItems.push_back(item);
    lostItemMap[item.id] = item;

    cout << "Lost item successfully registered!\n";
    cout << "Item ID: " << item.id << endl;
}

void LostFoundSystem::addFoundItem(Item item)
{
    if (!validateItem(item))
        return;

    if (checkDuplicate(item, false))
    {
        cout << "Item already reported found!\n";
        return;
    }

    item.id = generateId();
    item.status = "FOUND";

    foundItemKeys.insert(createItemKey(item));
    foundItems.push_back(item);
    foundItemMap[item.id] = item;

    cout << "Found item successfully registered!\n";
    cout << "Item ID: " << item.id << endl;
}

void LostFoundSystem::displayLostItems()
{
    bool any = false;
    for (const Item& item : lostItems)
    {
        if (item.status != "LOST")
            continue;
        any = true;
        cout << "\n";
        printItem(item);
    }
    if (!any)
        cout << "No active lost items reported!\n";
}

void LostFoundSystem::displayFoundItems()
{
    bool any = false;
    for (const Item& item : foundItems)
    {
        if (item.status != "FOUND")
            continue;
        any = true;
        cout << "\n";
        printItem(item);
    }
    if (!any)
        cout << "No active found items reported!\n";
}

bool LostFoundSystem::deleteLostItem(int id)
{
    auto it = find_if(lostItems.begin(), lostItems.end(),
                      [id](const Item& i) { return i.id == id; });

    if (it == lostItems.end())
    {
        cout << "Lost item not found!\n";
        return false;
    }

    // Only active items own a key; a recovered item's key was already freed
    if (it->status == "LOST")
        lostItemKeys.erase(createItemKey(*it));

    for (Claim& c : claims)
        if (c.lostItemId == id && c.status == "PENDING")
            c.status = "CANCELLED";

    lostItemMap.erase(id);
    lostItems.erase(it);

    cout << "Lost item deleted successfully!\n";
    return true;
}

bool LostFoundSystem::deleteFoundItem(int id)
{
    auto it = find_if(foundItems.begin(), foundItems.end(),
                      [id](const Item& i) { return i.id == id; });

    if (it == foundItems.end())
    {
        cout << "Found item not found!\n";
        return false;
    }

    if (it->status == "FOUND")
        foundItemKeys.erase(createItemKey(*it));

    for (Claim& c : claims)
        if (c.foundItemId == id && c.status == "PENDING")
            c.status = "CANCELLED";

    foundItemMap.erase(id);
    foundItems.erase(it);

    cout << "Found item deleted successfully!\n";
    return true;
}

// SEARCHING
// O(1) lookup through the hash maps
Item* LostFoundSystem::searchById(int id)
{
    auto lostIt = lostItemMap.find(id);
    if (lostIt != lostItemMap.end())
        return &lostIt->second;

    auto foundIt = foundItemMap.find(id);
    if (foundIt != foundItemMap.end())
        return &foundIt->second;

    cout << "Item not found!\n";
    return nullptr;
}

void LostFoundSystem::searchByName(string name)
{
    name = toLowerCase(trim(name));
    if (name.empty())
    {
        cout << "Please enter a search term!\n";
        return;
    }

    auto pred = [&](const Item& i)
    {
        return toLowerCase(i.name).find(name) != string::npos;
    };

    bool found = printMatching(lostItems, "LOST", "Lost", pred);
    found = printMatching(foundItems, "FOUND", "Found", pred) || found;

    if (!found)
        cout << "No item found with that name!\n";
}

void LostFoundSystem::searchByCategory(string category)
{
    category = toLowerCase(trim(category));
    if (category.empty())
    {
        cout << "Please enter a search term!\n";
        return;
    }

    auto pred = [&](const Item& i)
    {
        return toLowerCase(i.category).find(category) != string::npos;
    };

    bool found = printMatching(lostItems, "LOST", "Lost", pred);
    found = printMatching(foundItems, "FOUND", "Found", pred) || found;

    if (!found)
        cout << "No item found in that category!\n";
}

void LostFoundSystem::searchByLocation(string location)
{
    location = toLowerCase(trim(location));
    if (location.empty())
    {
        cout << "Please enter a search term!\n";
        return;
    }

    auto pred = [&](const Item& i)
    {
        return toLowerCase(i.location).find(location) != string::npos;
    };

    bool found = printMatching(lostItems, "LOST", "Lost", pred);
    found = printMatching(foundItems, "FOUND", "Found", pred) || found;

    if (!found)
        cout << "No item found at that location!\n";
}

// ==========================================
// MATCHING  (max score = 30+20+20+20+10 = 100)
// ==========================================

int LostFoundSystem::compareName(const Item& lost, const Item& found) const
{
    string n1 = toLowerCase(lost.name);
    string n2 = toLowerCase(found.name);
    if (n1 == n2)
        return 30;
    if (!n1.empty() && !n2.empty() &&
        (n1.find(n2) != string::npos || n2.find(n1) != string::npos))
        return 15;
    return 0;
}

int LostFoundSystem::compareCategory(const Item& lost, const Item& found) const
{
    return toLowerCase(lost.category) == toLowerCase(found.category) ? 20 : 0;
}

int LostFoundSystem::compareColor(const Item& lost, const Item& found) const
{
    string c1 = toLowerCase(lost.color);
    string c2 = toLowerCase(found.color);
    if (c1 == c2)
        return 20;
    if (!c1.empty() && !c2.empty() &&
        (c1.find(c2) != string::npos || c2.find(c1) != string::npos))
        return 10;
    return 0;
}

int LostFoundSystem::compareLocation(const Item& lost, const Item& found) const
{
    string l1 = toLowerCase(lost.location);
    string l2 = toLowerCase(found.location);
    if (l1 == l2)
        return 20;
    if (!l1.empty() && !l2.empty() &&
        (l1.find(l2) != string::npos || l2.find(l1) != string::npos))
        return 10;
    return 0;
}

int LostFoundSystem::compareDate(const Item& lost, const Item& found) const
{
    return lost.date == found.date ? 10 : 0;
}

int LostFoundSystem::calculateMatchScore(const Item& lost, const Item& found) const
{
    return compareName(lost, found) +
           compareCategory(lost, found) +
           compareColor(lost, found) +
           compareLocation(lost, found) +
           compareDate(lost, found);
}

void LostFoundSystem::displayBestMatches(
    priority_queue<MatchResult, vector<MatchResult>, CompareMatch>& matches)
{
    if (matches.empty())
    {
        cout << "No suitable matches found!\n";
        return;
    }

    while (!matches.empty())
    {
        MatchResult result = matches.top();
        matches.pop();

        cout << "\nMatch Score: " << result.score << "/100\n";
        cout << "Lost Item ID: " << result.lostItemId << endl;
        cout << "Found Item ID: " << result.foundItemId << endl;

        auto it = foundItemMap.find(result.foundItemId);   // O(1)
        if (it != foundItemMap.end())
        {
            cout << "Found Item Name: " << it->second.name << endl;
            cout << "Found Item Color: " << it->second.color << endl;
            cout << "Found Item Location: " << it->second.location << endl;
        }
        cout << "-------------------------\n";
    }
}

void LostFoundSystem::findMatches()
{
    bool anyLostItem = false;

    for (const Item& lost : lostItems)
    {
        if (lost.status != "LOST")
            continue;

        anyLostItem = true;

        cout << "\n================================\n";
        cout << "Matches for Lost Item: " << lost.name;
        cout << "\nLost Item ID: " << lost.id;
        cout << "\n================================\n";

        priority_queue<MatchResult, vector<MatchResult>, CompareMatch> pq;

        for (const Item& found : foundItems)
        {
            if (found.status != "FOUND")
                continue;

            int score = calculateMatchScore(lost, found);
            if (score >= MATCH_THRESHOLD)
                pq.push({lost.id, found.id, score});
        }
        displayBestMatches(pq);
    }

    if (!anyLostItem)
        cout << "No active lost items available for matching!\n";
}

// CLAIMS
void LostFoundSystem::createClaim(int lostItemId, int foundItemId,
                                  int claimantId, int finderId)
{
    auto lostIt  = lostItemMap.find(lostItemId);
    auto foundIt = foundItemMap.find(foundItemId);

    if (lostIt == lostItemMap.end() || foundIt == foundItemMap.end() ||
        lostIt->second.status != "LOST" || foundIt->second.status != "FOUND")
    {
        cout << "Invalid item IDs or inactive item!\n";
        return;
    }

    // Identity checks: IDs must belong to the people who reported the items
    if (lostIt->second.reporterId != claimantId)
    {
        cout << "Claimant ID does not match the person who reported this lost item!\n";
        return;
    }
    if (foundIt->second.reporterId != finderId)
    {
        cout << "Finder ID does not match the person who reported this found item!\n";
        return;
    }

    for (const Claim& claim : claims)
    {
        if (claim.lostItemId == lostItemId &&
            claim.foundItemId == foundItemId &&
            claim.status == "PENDING")
        {
            cout << "A pending claim already exists!\n";
            return;
        }
    }

    Claim newClaim;
    newClaim.claimId     = generateClaimId();
    newClaim.lostItemId  = lostItemId;
    newClaim.foundItemId = foundItemId;
    newClaim.claimantId  = claimantId;
    newClaim.finderId    = finderId;
    newClaim.status      = "PENDING";
    claims.push_back(newClaim);

    cout << "Claim created successfully!\n";
    cout << "Claim ID: " << newClaim.claimId << endl;
}

void LostFoundSystem::displayClaims(int finderId)
{
    bool any = false;

    for (const Claim& claim : claims)
    {
        if (claim.finderId != finderId)
            continue;

        any = true;
        cout << "\nClaim ID: " << claim.claimId;
        cout << "\nLost Item ID: " << claim.lostItemId;
        cout << "\nFound Item ID: " << claim.foundItemId;
        cout << "\nClaimant ID: " << claim.claimantId;
        cout << "\nStatus: " << claim.status;
        cout << "\n-------------------------\n";
    }

    if (!any)
        cout << "No claims found!\n";
}

// Found item handed back: no longer active, so free its duplicate key
void LostFoundSystem::markAsReturned(int foundItemId)
{
    for (Item& item : foundItems)
    {
        if (item.id == foundItemId && item.status == "FOUND")
        {
            foundItemKeys.erase(createItemKey(item));
            item.status = "RETURNED";
            foundItemMap[foundItemId] = item;
            return;
        }
    }
}

// Lost report resolved: no longer active, so free its duplicate key
void LostFoundSystem::markAsRecovered(int lostItemId)
{
    for (Item& item : lostItems)
    {
        if (item.id == lostItemId && item.status == "LOST")
        {
            lostItemKeys.erase(createItemKey(item));
            item.status = "RECOVERED";
            lostItemMap[lostItemId] = item;
            return;
        }
    }
}

void LostFoundSystem::acceptClaim(int claimId, int finderId)
{
    for (Claim& claim : claims)
    {
        if (claim.claimId != claimId || claim.finderId != finderId)
            continue;

        if (claim.status != "PENDING")
        {
            cout << "Claim already processed!\n";
            return;
        }

        auto foundIt = foundItemMap.find(claim.foundItemId);
        if (foundIt == foundItemMap.end() || foundIt->second.status != "FOUND")
        {
            cout << "Found item is no longer available!\n";
            return;
        }

        auto lostIt = lostItemMap.find(claim.lostItemId);
        if (lostIt == lostItemMap.end() || lostIt->second.status != "LOST")
        {
            cout << "Lost item report is no longer active!\n";
            return;
        }

        claim.status = "ACCEPTED";
        markAsReturned(claim.foundItemId);
        markAsRecovered(claim.lostItemId);

        // Close every other pending claim that involves either item
        for (Claim& other : claims)
        {
            if (other.claimId != claim.claimId &&
                other.status == "PENDING" &&
                (other.foundItemId == claim.foundItemId ||
                 other.lostItemId == claim.lostItemId))
            {
                other.status = "REJECTED";
            }
        }

        cout << "Claim accepted successfully!\n";
        return;
    }

    cout << "Claim not found or you are not its finder!\n";
}

void LostFoundSystem::rejectClaim(int claimId, int finderId)
{
    for (Claim& claim : claims)
    {
        if (claim.claimId != claimId || claim.finderId != finderId)
            continue;

        if (claim.status != "PENDING")
        {
            cout << "Claim already processed!\n";
            return;
        }

        claim.status = "REJECTED";
        cout << "Claim rejected successfully!\n";
        cout << "Found item remains available.\n";
        return;
    }

    cout << "Claim not found or you are not its finder!\n";
}

// UTILITY

void LostFoundSystem::sortItems(vector<Item>& items, string criterion)
{
    if (criterion == "date")
    {
        sort(items.begin(), items.end(),
            [](const Item& a, const Item& b) { return a.date < b.date; });
    }
    else if (criterion == "location")
    {
        sort(items.begin(), items.end(),
            [](const Item& a, const Item& b) { return a.location < b.location; });
    }
    else
    {
        cout << "Invalid sorting criterion!\n";
        return;
    }
    cout << "Items sorted successfully!\n";
}