#include "LostFoundSystem.h"
#include<bits/stdc++.h>
void LostFoundSystem::addLostItem(Item item){
    if(item.name.empty() || item.color.empty() || item.category.empty() ||item.location.empty() || item.description.empty()){
        cout<<"Please fill all the Required details !\n";
        return;
    }
    item.id=generateId();
    item.status="LOST";
    if(checkDuplicate(item)){
        cout<<"Item already reported lost";
        return;
    }
    lostItems.push_back(item);
    cout<<"Lost Item Successfully Registered !\n";
    cout<<"Item Id : "<<item.id;
    return;
}