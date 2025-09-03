#pragma once
#include <windows.h>
#include <string>

// Control IDs
#define IDC_TITLE           1001
#define IDC_LABEL_MOBILE    1002
#define IDC_MOBILE          1003
#define IDC_LABEL_NAME      1004
#define IDC_NAME            1005
#define IDC_EBILL_BTN       1006
#define IDC_PAPER_BILL      1007
#define IDC_BOTH_BILL       1008

// Customer information structure
struct CustomerInfo {
    std::wstring mobile;
    std::wstring name;
    BOOL paperBill= FALSE;
    BOOL bothBill = FALSE;
};

// Function declarations
bool ShowBillBoxDialog(CustomerInfo& info, HWND hParent = nullptr);
INT_PTR CALLBACK BillBoxDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
 
