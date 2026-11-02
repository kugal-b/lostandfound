#include "LostFoundSystem.h"
#include <iostream>
#include <string>
#include <limits>

using namespace std;

int main()
{
  LostFoundSystem system;

  // Create sample lost items
  Item item1;
  item1.id = 1;
  item1.name = "Black Wallet";
  item1.category = "Accessories";
  item1.color = "Black";
  item1.location = "Library";
  item1.date = "01-11-2026";
  item1.description = "Leather wallet";
  item1.status = "LOST";

  Item item2;
  item2.id = 2;
  item2.name = "Red Bag";
  item2.category = "Accessories";
  item2.color = "Red";
  item2.location = "Library Ground Floor";
  item2.date = "02-11-2026";
  item2.description = "Small red backpack";
  item2.status = "LOST";

  // Create sample found items
  Item item3;
  item3.id = 3;
  item3.name = "Blue Bottle";
  item3.category = "Bottle";
  item3.color = "Blue";
  item3.location = "Canteen";
  item3.date = "01-11-2026";
  item3.description = "Steel water bottle";
  item3.status = "FOUND";

  Item item4;
  item4.id = 4;
  item4.name = "Black Wallet";
  item4.category = "Accessories";
  item4.color = "Black";
  item4.location = "Sports Ground";
  item4.date = "03-11-2026";
  item4.description = "Wallet found near the ground";
  item4.status = "FOUND";

  // Add items to the system
  system.addLostItem(item1);
  system.addLostItem(item2);

  system.addFoundItem(item3);
  system.addFoundItem(item4);

  int choice;

  do {
    cout << "\n===== SEARCH SYSTEM TEST =====\n";
    cout << "1. Search by ID\n";
    cout << "2. Search by Name\n";
    cout << "3. Search by Category\n";
    cout << "4. Search by Location\n";
    cout << "5. Exit\n";
    cout << "Enter your choice: ";
    cin >> choice;

    if (choice == 1) {
      int id;

      cout << "Enter item ID: ";
      cin >> id;

      Item* result = system.searchById(id);

      if (result != nullptr) {
        cout << "\nItem Found!\n";
        cout << "ID: " << result->id << endl;
        cout << "Name: " << result->name << endl;
        cout << "Category: " << result->category << endl;
        cout << "Color: " << result->color << endl;
        cout << "Location: " << result->location << endl;
        cout << "Status: " << result->status << endl;
      }
      else {
        cout << "No item found with ID: " << id << endl;
      }
    }

    else if (choice == 2) {
      string name;

      cout << "Enter item name: ";
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      getline(cin, name);

      system.searchByName(name);
    }

    else if (choice == 3) {
      string category;

      cout << "Enter category: ";
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      getline(cin, category);

      system.searchByCategory(category);
    }

    else if (choice == 4) {
      string location;

      cout << "Enter location: ";
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      getline(cin, location);

      system.searchByLocation(location);
    }

    else if (choice == 5) {
      cout << "Exiting search test...\n";
    }

    else {
      cout << "Invalid choice. Try again.\n";
    }

  } while (choice != 5);

  return 0;
}
