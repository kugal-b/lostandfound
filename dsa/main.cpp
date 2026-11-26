#include <iostream>
#include <string>
#include <cstdlib>
#include <cctype>
#include <stdexcept>
#include "LostFoundSystem.h"

using namespace std;
// INPUT HELPERS
string readLine(const string& message)
{
    string line;
    cout << message;

    if (!getline(cin, line))
    {
        cout << "\nInput closed. Exiting...\n";
        exit(0);
    }
    return line;
}

// Accepts only a whole line that is a valid integer ("12abc" is rejected)
int getIntegerInput(const string& message)
{
    while (true)
    {
        string line = readLine(message);

        try
        {
            size_t pos = 0;
            int value = stoi(line, &pos);

            while (pos < line.size() &&
                   isspace(static_cast<unsigned char>(line[pos])))
                pos++;

            if (pos == line.size())
                return value;
        }
        catch (const exception&)
        {
            // fall through to the error message
        }

        cout << "Invalid input! Please enter a number.\n";
    }
}

// TAKE ITEM DETAILS
Item inputItem()
{
    Item item;

    cout << "\n========== ENTER ITEM DETAILS ==========\n";

    item.reporterId = getIntegerInput("Enter your User ID: ");
    item.name        = readLine("Enter item name: ");
    item.category    = readLine("Enter category: ");
    item.color       = readLine("Enter color: ");
    item.location    = readLine("Enter location: ");
    item.date        = readLine("Enter date (YYYY-MM-DD): ");
    item.description = readLine("Enter description: ");

    return item;
}

// SEARCH SUBMENU
void searchMenu(LostFoundSystem& app)
{
    int choice;

    do
    {
        cout << "\n========== SEARCH ITEMS ==========\n";
        cout << "1. Search by ID\n";
        cout << "2. Search by Name\n";
        cout << "3. Search by Category\n";
        cout << "4. Search by Location\n";
        cout << "5. Back\n";

        choice = getIntegerInput("Enter your choice: ");

        switch (choice)
        {
            case 1:
            {
                int id = getIntegerInput("Enter Item ID: ");
                Item* item = app.searchById(id);

                if (item != nullptr)
                {
                    cout << "\nItem Found!\n";
                    cout << "ID: " << item->id << endl;
                    cout << "Name: " << item->name << endl;
                    cout << "Category: " << item->category << endl;
                    cout << "Color: " << item->color << endl;
                    cout << "Location: " << item->location << endl;
                    cout << "Date: " << item->date << endl;
                    cout << "Description: " << item->description << endl;
                    cout << "Status: " << item->status << endl;
                }
                break;
            }

            case 2:
                app.searchByName(readLine("Enter item name: "));
                break;

            case 3:
                app.searchByCategory(readLine("Enter category: "));
                break;

            case 4:
                app.searchByLocation(readLine("Enter location: "));
                break;

            case 5:
                cout << "Returning to main menu...\n";
                break;

            default:
                cout << "Invalid choice!\n";
        }

    } while (choice != 5);
}

// CLAIM MANAGEMENT SUBMENU

void claimMenu(LostFoundSystem& app)
{
    int choice;

    do
    {
        cout << "\n========== CLAIM MANAGEMENT ==========\n";
        cout << "1. View Claims\n";
        cout << "2. Create Claim\n";
        cout << "3. Accept Claim\n";
        cout << "4. Reject Claim\n";
        cout << "5. Back\n";

        choice = getIntegerInput("Enter your choice: ");

        switch (choice)
        {
            case 1:
            {
                int finderId = getIntegerInput("Enter your Finder ID: ");
                app.displayClaims(finderId);
                break;
            }

            case 2:
            {
                int lostId     = getIntegerInput("Enter Lost Item ID: ");
                int foundId    = getIntegerInput("Enter Found Item ID: ");
                int claimantId = getIntegerInput("Enter your Claimant ID: ");
                int finderId   = getIntegerInput("Enter Finder ID: ");

                app.createClaim(lostId, foundId, claimantId, finderId);
                break;
            }

            case 3:
            {
                int claimId  = getIntegerInput("Enter Claim ID: ");
                int finderId = getIntegerInput("Enter your Finder ID: ");
                app.acceptClaim(claimId, finderId);
                break;
            }

            case 4:
            {
                int claimId  = getIntegerInput("Enter Claim ID: ");
                int finderId = getIntegerInput("Enter your Finder ID: ");
                app.rejectClaim(claimId, finderId);
                break;
            }

            case 5:
                cout << "Returning to main menu...\n";
                break;

            default:
                cout << "Invalid choice!\n";
        }

    } while (choice != 5);
}

// MAIN FUNCTION
int main()
{
    LostFoundSystem app;   // not named "system": that would shadow std::system

    int choice;

    do
    {
        cout << "\n";
        cout << "============================================\n";
        cout << "          LOST AND FOUND SYSTEM             \n";
        cout << "============================================\n";
        cout << "1. Report Lost Item\n";
        cout << "2. Report Found Item\n";
        cout << "3. Display Lost Items\n";
        cout << "4. Display Found Items\n";
        cout << "5. Search Item\n";
        cout << "6. Find Matches\n";
        cout << "7. Manage Claims\n";
        cout << "8. Exit\n";
        cout << "============================================\n";

        choice = getIntegerInput("Enter your choice: ");

        switch (choice)
        {
            case 1:
                app.addLostItem(inputItem());
                break;

            case 2:
                app.addFoundItem(inputItem());
                break;

            case 3:
                app.displayLostItems();
                break;

            case 4:
                app.displayFoundItems();
                break;

            case 5:
                searchMenu(app);
                break;

            case 6:
                app.findMatches();
                break;

            case 7:
                claimMenu(app);
                break;

            case 8:
                cout << "\nThank you for using Lost and Found System!\n";
                cout << "Exiting program...\n";
                break;

            default:
                cout << "Invalid choice! Please try again.\n";
        }

    } while (choice != 8);

    return 0;
}