#pragma once  
#include <string>
#include <windows.h>  

// Structure to hold dialog result  
struct CustomerInfo {
    std::wstring mobile;
    std::wstring name;
};

bool ShowBillBoxDialog(CustomerInfo& outInfo, HWND hParent = nullptr);
