#include <iostream>
#include "TUI.h"

using namespace std;

int main()
{
    LostFoundSystem system;

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
    item2.description = "Small backpack";
    item2.status = "LOST";

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
    item4.description = "Wallet found near ground";
    item4.status = "FOUND";

    system.addLostItem(item1);
    system.addLostItem(item2);
    system.addFoundItem(item3);
    system.addFoundItem(item4);

    RunTUI(system);

    return 0;
}
