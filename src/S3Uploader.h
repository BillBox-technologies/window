#pragma once
#include <string>

// Returns true if upload succeeded; responseOut will have the server's response.
bool UploadPdfToBillBox(const std::wstring& pdfPath, const std::wstring& mobile, const std::wstring& filename, std::wstring& responseOut, std::wstring& errorOut,BOOL paperBill);
