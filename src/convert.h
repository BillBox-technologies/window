#pragma once
#include <string>

// Returns true if PDF created successfully
bool ConvertPsToPdf(const std::wstring& inputPs, const std::wstring& outputPdf, std::wstring& errorMsg);
