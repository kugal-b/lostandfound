#include "TUI.h"
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

int main()
{
  LostFoundSystem system;

  // Create sample items for testing the browse and search screens.
  // addLostItem() and addFoundItem() generate IDs and set statuses.

  auto makeItem = [](string name, string category, string color,
                     string location, string date, string description) {
    Item item;
    item.name = name;
    item.category = category;
    item.color = color;
    item.location = location;
    item.date = date;
    item.description = description;
    return item;
  };

  // Lost items
  Item lost1 = makeItem("Black Wallet", "Accessories", "Black",
                        "Central Library", "01-11-2026",
                        "Black leather wallet with a card holder");
  Item lost2 = makeItem("Blue Backpack", "Bags", "Blue",
                        "Main Canteen", "02-11-2026",
                        "Blue backpack containing notebooks");
  Item lost3 = makeItem("iPhone 14", "Electronics", "White",
                        "Computer Lab 2", "02-11-2026",
                        "White phone with a clear case");
  Item lost4 = makeItem("Silver Keys", "Personal Items", "Silver",
                        "Parking Area", "03-11-2026",
                        "Three keys attached to a round keychain");
  Item lost5 = makeItem("Water Bottle", "Bottles", "Green",
                        "Sports Complex", "03-11-2026",
                        "Green reusable steel bottle");
  Item lost6 = makeItem("Student ID Card", "Documents", "White",
                        "Academic Block", "04-11-2026",
                        "University identity card in a transparent cover");

  // Found items
  Item found1 = makeItem("Black Leather Wallet", "Accessories", "Black",
                         "Sports Ground", "02-11-2026",
                         "Wallet found near the football field");
  Item found2 = makeItem("Blue School Bag", "Bags", "Navy Blue",
                         "Library Entrance", "03-11-2026",
                         "Navy blue bag with two front pockets");
  Item found3 = makeItem("Android Phone", "Electronics", "Black",
                         "Lecture Hall 4", "03-11-2026",
                         "Black phone found below a classroom desk");
  Item found4 = makeItem("Key Ring", "Personal Items", "Golden",
                         "Main Gate", "04-11-2026",
                         "Small key ring with two keys");
  Item found5 = makeItem("Red Water Bottle", "Bottles", "Red",
                         "Main Canteen", "04-11-2026",
                         "Red bottle left on a table");
  Item found6 = makeItem("Library Card", "Documents", "Blue",
                         "Central Library", "04-11-2026",
                         "Library membership card");
  Item found7 = makeItem("Wireless Earbuds", "Electronics", "White",
                         "Auditorium", "05-11-2026",
                         "White earbuds in a charging case");
  Item found8 = makeItem("Grey Hoodie", "Clothing", "Grey",
                         "Basketball Court", "05-11-2026",
                         "Grey hoodie found on the spectator seats");

  // Keep startup sample-registration messages out of the TUI.
  ostringstream startupOutput;
  streambuf* oldOutput = cout.rdbuf(startupOutput.rdbuf());

  system.addLostItem(lost1);
  system.addLostItem(lost2);
  system.addLostItem(lost3);
  system.addLostItem(lost4);
  system.addLostItem(lost5);
  system.addLostItem(lost6);

  system.addFoundItem(found1);
  system.addFoundItem(found2);
  system.addFoundItem(found3);
  system.addFoundItem(found4);
  system.addFoundItem(found5);
  system.addFoundItem(found6);
  system.addFoundItem(found7);
  system.addFoundItem(found8);

  cout.rdbuf(oldOutput);

  RunTUI(system);
  return 0;
}
