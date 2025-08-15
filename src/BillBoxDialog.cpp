
#include "BillBoxDialog.h"
#include <tchar.h>

static CustomerInfo* g_pInfo = nullptr;

DLGTEMPLATE* CreateDialogTemplate(HGLOBAL& hTemplate) {
    const int itemCount = 6;

    hTemplate = GlobalAlloc(GPTR, 4096);
    BYTE* p = (BYTE*)GlobalLock(hTemplate);
    DLGTEMPLATE* dlg = (DLGTEMPLATE*)p;

    dlg->style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_SETFONT | WS_VISIBLE;
    dlg->cdit = itemCount;
    dlg->x = 10; dlg->y = 10; dlg->cx = 220; dlg->cy = 90;

    p += sizeof(DLGTEMPLATE);
    *(WORD*)p = 0; p += 2; // No menu
    *(WORD*)p = 0; p += 2; // Default class
    wcscpy((WCHAR*)p, L"BillBox"); p += (wcslen(L"BillBox") + 1) * 2;
    *(WORD*)p = 13; p += 2; // Font size
    wcscpy((WCHAR*)p, L"Segoe UI"); p += (wcslen(L"Segoe UI") + 1) * 2;
    auto AddControl = [&](DWORD style, WORD id, WORD x, WORD y, WORD cx, WORD cy, WORD classAtom, const wchar_t* title) {
        p = (BYTE*)(((uintptr_t)p + 3) & ~3);
        DLGITEMTEMPLATE* item = (DLGITEMTEMPLATE*)p;
        item->style = WS_CHILD | WS_VISIBLE | style;
        item->x = x; item->y = y; item->cx = cx; item->cy = cy;
        item->id = id;
        p += sizeof(DLGITEMTEMPLATE);
        *(WORD*)p = 0xFFFF; p += 2;
        *(WORD*)p = classAtom; p += 2;
        wcscpy((WCHAR*)p, title); p += (wcslen(title) + 1) * 2;
        *(WORD*)p = 0; p += 2;
    };

    AddControl(SS_LEFT,      IDC_TITLE,         80, 8,   60, 12, 0x82, L"BillBox");
    AddControl(SS_LEFT,      IDC_LABEL_MOBILE,  10, 28,  40, 10, 0x82, L"Mobile:");
    AddControl(ES_AUTOHSCROLL | WS_BORDER, IDC_MOBILE, 60, 26, 140, 12, 0x81, L"");
    AddControl(SS_LEFT,      IDC_LABEL_NAME,    10, 46,  40, 10, 0x82, L"Name:");
    AddControl(ES_AUTOHSCROLL | WS_BORDER, IDC_NAME,   60, 44, 140, 12, 0x81, L"");
    AddControl(BS_PUSHBUTTON, IDC_EBILL_BTN,    75, 64,  60, 14, 0x80, L"E-Bill");

    GlobalUnlock(hTemplate);
    return (DLGTEMPLATE*)hTemplate;
}

INT_PTR CALLBACK DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    static bool handlingInput = false;

    switch (msg) {
    case WM_INITDIALOG:{
    g_pInfo = reinterpret_cast<CustomerInfo*>(lParam);

    // Position as before...
    HWND hwndOwner = GetForegroundWindow();
    if (!hwndOwner) hwndOwner = GetDesktopWindow();

    RECT rcOwner, rcDlg;
    GetWindowRect(hwndOwner, &rcOwner);
    GetWindowRect(hDlg, &rcDlg);

    int dlgWidth = rcDlg.right - rcDlg.left;
    int dlgHeight = rcDlg.bottom - rcDlg.top;
    int x = rcOwner.left + ((rcOwner.right - rcOwner.left) - dlgWidth) / 2;
    int y = rcOwner.top + ((rcOwner.bottom - rcOwner.top) - dlgHeight) / 2 - 40;

    //Button Should Start Disabled
    EnableWindow(GetDlgItem(hDlg, IDC_EBILL_BTN), FALSE);

    // TOPMOST
    SetWindowPos(hDlg, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);

    // ATTACH THREAD TRICK
    DWORD fgThread = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    DWORD curThread = GetCurrentThreadId();
    if (fgThread != curThread) {
        AttachThreadInput(fgThread, curThread, TRUE);
        SetForegroundWindow(hDlg);
        AttachThreadInput(fgThread, curThread, FALSE);
    }
    else {
        SetForegroundWindow(hDlg);
    }
    BringWindowToTop(hDlg);
    SetActiveWindow(hDlg);

    // Optionally, set timer for a few seconds to re-assert topmost
    SetTimer(hDlg, 1, 100, NULL); // 100 ms

    // Flash
    FLASHWINFO fi = { sizeof(fi) };
    fi.hwnd = hDlg;
    fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
    fi.uCount = 3;
    fi.dwTimeout = 0;
    FlashWindowEx(&fi);

SetFocus(GetDlgItem(hDlg, IDC_MOBILE));
      return FALSE;
}

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_MOBILE && HIWORD(wParam) == EN_CHANGE && !handlingInput) {
            handlingInput = true;
            TCHAR buf[32];
            GetDlgItemText(hDlg, IDC_MOBILE, buf, 32);

            TCHAR filtered[16] = {};
            int k = 0;
            for (int i = 0; buf[i] && k < 10; ++i)
                if (buf[i] >= _T('0') && buf[i] <= _T('9'))
                    filtered[k++] = buf[i];

            EnableWindow(GetDlgItem(hDlg, IDC_EBILL_BTN), k == 10);
            if (lstrcmp(buf, filtered) != 0)
                SetDlgItemText(hDlg, IDC_MOBILE, filtered);
            handlingInput = false;
        }

        if (LOWORD(wParam) == IDC_EBILL_BTN) {
            TCHAR mobile[32], name[64];
            GetDlgItemText(hDlg, IDC_MOBILE, mobile, 32);
            GetDlgItemText(hDlg, IDC_NAME, name, 64);

            if (g_pInfo) {
                g_pInfo->mobile = std::wstring(mobile,mobile+lstrlen(mobile));
                g_pInfo->name= std::wstring(name,name+lstrlen(name));
            }
            EndDialog(hDlg, IDOK);
            return TRUE;
        }

        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
    }

    return FALSE;
}

bool ShowBillBoxDialog(CustomerInfo& info, HWND hParent) {
    HGLOBAL hTemplate = nullptr;
    DLGTEMPLATE* dlg = CreateDialogTemplate(hTemplate);

    INT_PTR ret = DialogBoxIndirectParamW(
        GetModuleHandle(NULL),
        dlg,
        hParent,
        DialogProc,
        (LPARAM)&info
    );

    GlobalFree(hTemplate);
    return ret == IDOK;
}

