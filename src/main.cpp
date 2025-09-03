#include <windows.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <string>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <winspool.h>
#include <vector>
#include <shlwapi.h>

#define CURL_STATICLIB
#include <curl.h>

#include "BillBoxDialog.h"
#include "convert.h"
#include "S3Uploader.h"

std::wstring generatedPdfPath;
std::wstring conversionError;
bool conversionDone = false;

const std::wstring inputPs = L"C:\\BillBox\\Prints\\print.ps";
const std::wstring tempPdf = L"C:\\BillBox\\Bills\\TEMP_CONVERTED.pdf";
const std::wstring outputFolder = L"C:\\BillBox\\Bills\\";


bool RawForwardToPrinter(const std::wstring& printerName, const std::wstring& filePath);

void BackgroundConvert() {
    std::wstring err;
    if (ConvertPsToPdf(inputPs, tempPdf, err)) {
        conversionDone = true;
        generatedPdfPath = tempPdf;
    } else {
        conversionDone = false;
        conversionError = err;
    }
}
bool DeletePdf(const std::wstring& pdfPath) {
    if (DeleteFileW(pdfPath.c_str())) {
        return true; // Successfully deleted
    } else {
        // Optional: You can get extended error info here if needed
        return false; // Deletion failed
    }
}
bool WaitForNextFileChange(const std::wstring& filePath) {
    FILETIME lastWrite = {};
    WIN32_FILE_ATTRIBUTE_DATA attr;

    // Get the current write time at startup
    if (!GetFileAttributesExW(filePath.c_str(), GetFileExInfoStandard, &attr)) {
      //  MessageBoxW(NULL, L"❌ File does not exist:\n" + filePath.c_str(), L"BillBox", MB_OK | MB_ICONERROR);
        return false;
    }

    lastWrite = attr.ftLastWriteTime;

    // Arm only after first change
    bool armed = false;

    while (true) {
        Sleep(100);
        if (GetFileAttributesExW(filePath.c_str(), GetFileExInfoStandard, &attr)) {
            if (CompareFileTime(&lastWrite, &attr.ftLastWriteTime) != 0) {
                lastWrite = attr.ftLastWriteTime;

                if (!armed) {
                    // First change → just arm
                    armed = true;
                } else {
                    // Second change → actual trigger
                    return true;
                }
            }
        }
    }

    return false;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int) {

    curl_global_init(CURL_GLOBAL_ALL);
	//WaitForNextFileChange(inputPs);
    std::thread convertThread(BackgroundConvert);

    CustomerInfo info;
    bool accepted = ShowBillBoxDialog(info,NULL);

    convertThread.join();

    if(info.paperBill || info.bothBill)
        RawForwardToPrinter(L"forward-test", L"D:\\test\\print.ps");
    if (!accepted) {
        //MessageBoxW(NULL, L"User cancelled input.", L"Cancelled", MB_OK | MB_ICONINFORMATION);
        curl_global_cleanup();
        return 0;
    }

    if (!conversionDone) {
        MessageBoxW(NULL, conversionError.c_str(), L"Conversion Error", MB_ICONERROR);
        curl_global_cleanup();
        return 1;
    }

    // Timestamp
    std::time_t now = std::time(nullptr);
    std::tm localTm{};
    localtime_s(&localTm, &now);
    wchar_t timeBuf[64];
    wcsftime(timeBuf, 64, L"%Y%m%d_%H%M%S", &localTm);

//    std::wstring s3Key = info.mobile + L"/" + timeBuf + L".pdf";
    std::wstring s3Key = L".pdf";
    std::wstring localDir = outputFolder + info.mobile + L"\\";
  //  std::wstring localPath = localDir + timeBuf + L".pdf";
    std::wstring localPath;
    localPath =  outputFolder +L"TEMP_CONVERTED.pdf";

    std::error_code ec;
  //  std::filesystem::create_directories(localDir, ec);


    // Move instead of copy
  //  MessageBoxW(NULL, generatedPdfPath.c_str(), L"File Error", MB_ICONERROR);
  //  MessageBoxW(NULL, localPath.c_str(), L"File Error", MB_ICONERROR);
    //  GeneratedPath=C:\\BillBox\\Bills\\TEMP.pdf
    //  localPath=C:\\BillBox\\Bills\\63010123152\\s3Key.pdf
    
    if (!MoveFileW(generatedPdfPath.c_str(), localPath.c_str())) {
        MessageBoxW(NULL, L"Failed to move PDF to local path.", L"File Error", MB_ICONERROR);
    //    MessageBoxW(NULL, L" entered MoveFileW(generatedPdfPath.c_str(), localPath.c_str()", L"File Error", MB_ICONERROR);
        
        curl_global_cleanup();
        return 1;
    }else{
    //	MessageBoxW(NULL,L"Sucess of Moving", L"File Error", MB_ICONERROR);
    }

    std::wstring s3Url, s3Error;

    if (!UploadPdfToBillBox(localPath, info.mobile, s3Key, s3Url, s3Error,info.paperBill)) {
    //    MessageBoxW(NULL, (L"Upload failed. PDF saved locally:\n" + localPath).c_str(),
     //               L"Upload Error", MB_ICONERROR);
        curl_global_cleanup();
       // MessageBoxW(NULL,L"End of Uploading", L"File Error", MB_ICONERROR);
       DeletePdf(localPath);
        return 1;
    }else{
    	//MessageBoxW(NULL, L"Sucess of Uploading", L"File Error", MB_ICONERROR);
    }




    curl_global_cleanup();
    return 0;
}

bool RawForwardToPrinter(const std::wstring& printerName, const std::wstring& filePath) {
    HANDLE hPrinter = NULL;
    if (!OpenPrinterW((LPWSTR)printerName.c_str(), &hPrinter, NULL)) {
        return false;
    }

    DOC_INFO_1W docInfo;
    docInfo.pDocName = (LPWSTR)L"BillBox Forward Job";
    docInfo.pOutputFile = NULL;
    docInfo.pDatatype = (LPWSTR)L"RAW";

    bool success = false;
    if (StartDocPrinterW(hPrinter, 1, (LPBYTE)&docInfo)) {
        if (StartPagePrinter(hPrinter)) {
            std::ifstream file(filePath, std::ios::binary);
            if (file) {
                std::vector<char> buffer((std::istreambuf_iterator<char>(file)), {});
                DWORD bytesWritten = 0;
                if (WritePrinter(hPrinter, buffer.data(), (DWORD)buffer.size(), &bytesWritten)) {
                    success = true;
                }
            }
            EndPagePrinter(hPrinter);
        }
        EndDocPrinter(hPrinter);
    }

    ClosePrinter(hPrinter);
    return success;
}
