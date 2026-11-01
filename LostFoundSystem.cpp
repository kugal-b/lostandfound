#include "LostFoundSystem.h"
#include<bits/stdc++.h>
//item management 
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

void LostFoundSystem::addFoundItem(Item item){
    if(item.name.empty() || item.color.empty() || item.category.empty() ||item.location.empty() || item.description.empty()){
        cout<<"Please fill all the Required details !\n";
        return;
    }
    item.id=generateId();
    item.status="FOUND";
    if(checkDuplicate(item)){
        cout<<"Item already reported lost";
        return;
    }
    foundItems.push_back(item);
    cout<<"Founded Item Successfully Registered !\n";
    cout<<"Item Id : "<<item.id;
    return;
}

void LostFoundSystem::displayLostItems(){
    if (lostItems.empty())
    {
        cout << "No lost items reported yet!\n";
        return;
    }
    for(int i=0;i<lostItems.size();i++){
        cout<< i+1<<".\n";
        cout<<"Item Name : "<<lostItems[i].name<<endl;
        cout<<"Item ID : "<<lostItems[i].id<<endl;
        cout<<"Item Category : "<<lostItems[i].category<<endl;
        cout<<"Item Color : "<<lostItems[i].color<<endl;
        cout<<"Item Location : "<<lostItems[i].location<<endl;
        cout<<"Item Date : "<<lostItems[i].date<<endl;
        cout<<"Item Description : "<<lostItems[i].description<<endl;
        cout<<"Item Status : "<<lostItems[i].status<<endl;
    }

void LostFoundSystem::displayFoundItems(){
    if (foundItems.empty())
    {
        cout << "No Founded items reported yet!\n";
        return;
    }
    for(int i=0;i<foundItems.size();i++){
        cout<< i+1<<".\n";
        cout<<"Item Name : "<<foundItems[i].name<<endl;
        cout<<"Item ID : "<<foundItems[i].id<<endl;
        cout<<"Item Category : "<<foundItems[i].category<<endl;
        cout<<"Item Color : "<<foundItems[i].color<<endl;
        cout<<"Item Location : "<<foundItems[i].location<<endl;
        cout<<"Item Date : "<<foundItems[i].date<<endl;
        cout<<"Item Description : "<<foundItems[i].description<<endl;
        cout<<"Item Status : "<<foundItems[i].status<<endl;
        cout<<endl;
    }
}

void LostFoundSystem :: deleteLostItem(int id){
    if(lostItem.empty()){
        cout << "No lost items reported yet!\n";
        return;
    }
    bool found=false;
    for(int i=0;i<lostItem.size();i++){
        if(lostItem[i].id==id){
            found=true;
            lostItem.erase(lostItem.begin()+i);
            cout << "Lost item deleted successfully!\n";
            return;
        }
    }
    if(!found){
        cout<<"No Lost item with ID : "<<id<<" reported\n";
        return;
    }
}

void LostFoundSystem :: deleteFoundItem(int id){
    if(foundItem.empty()){
        cout << "No Found items reported yet!\n";
        return;
    }
    bool found=false;
    for(int i=0;i<foundItem.size();i++){
        if(foundItem[i].id==id){
            found=true;
            foundItem.erase(foundItem.begin()+i);
            cout << "Found item deleted successfully!\n";
            return;
        }
    }
    if(!found){
        cout<<"No Found item with ID : "<<id<<" reported\n";
        return;
    }
}


//searching


//Matching functions