#include <windows.h>
#include <fstream>
#include "convert.h"

bool ConvertPsToPdf(const std::wstring& inputPs, const std::wstring& outputPdf, std::wstring& errorMsg) {
    std::wstring gsPath = L"C:\\BillBox\\common\\gs.exe";

    // Ensure input file exists
    //
std::wifstream inFile(inputPs.c_str());
    if (!inFile.good()) {
        errorMsg = L"Input file not found.";
        return false;
    }

    // Compose command line
    std::wstring cmd = L"\"" + gsPath + L"\" -sDEVICE=pdfwrite -o \"" + outputPdf + L"\" -dBATCH -dNOPAUSE -dSAFER \"" + inputPs + L"\"";

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // No window

    // Run Ghostscript process
    if (!CreateProcessW(
        NULL, (LPWSTR)cmd.c_str(),
        NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        errorMsg = L"Could not launch Ghostscript. Check path and permissions.";
        return false;
    }

    // Wait for process to finish
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // Check if PDF file created
    std::wifstream outFile(outputPdf.c_str());
    if (exitCode != 0 || !outFile.good()) {
        errorMsg = L"Ghostscript failed. PDF was not created.";
        return false;
    }

    return true;
}
