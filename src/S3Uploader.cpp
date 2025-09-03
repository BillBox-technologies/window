
#include <windows.h>
#include <fstream>
#include <string>
#include <shlwapi.h>
#include <CommCtrl.h>
#include <curl.h>
#include <fstream>
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "Shlwapi.lib")

// Resolve the directory of the running executable
static std::wstring GetExecutableDirectory() {
	wchar_t exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);
	std::wstring path(exePath);
	size_t lastSlash = path.find_last_not_of(L"\\/");
	if (lastSlash != std::wstring::npos) {
		// move to the actual slash character position
		lastSlash = path.find_last_of(L"\\/", lastSlash);
		if (lastSlash != std::wstring::npos) {
			return path.substr(0, lastSlash + 1);
		}
	}
	return L"";
}

// Read store ID from a config file placed next to the executable
std::wstring ReadStoreIDFromConfig() {
	std::wstring configPath = GetExecutableDirectory() + L"billbox_config.ini";
	std::wifstream configFile(configPath);
	std::wstring storeID;
	
	if (configFile.is_open()) {
		std::getline(configFile, storeID);
		// Safely trim trailing whitespace/newlines
		if (!storeID.empty()) {
			size_t end = storeID.find_last_not_of(L" \n\r\t");
			if (end != std::wstring::npos) {
				storeID.erase(end + 1);
			} else {
				storeID.clear();
			}
		}
	}
	
	return storeID;
}

// Upload using HTTP API instead of S3
bool UploadPdfToBillBox(const std::wstring& pdfPath, const std::wstring& mobile, const std::wstring& filename, std::wstring& responseOut, std::wstring& errorOut,BOOL paperBill) {
    if (!PathFileExistsW(pdfPath.c_str())) {
        std::wstring msg = L"❌ File does not exist:\n" + pdfPath;
        MessageBoxW(nullptr, msg.c_str(), L"BillBox Upload", MB_OK | MB_ICONERROR);
        errorOut = msg;
        return false;
    }

    if (mobile.empty() && paperBill==FALSE) {
        MessageBoxW(nullptr, L"❌ Missing mobile number (parameter is empty).", L"BillBox Upload", MB_OK | MB_ICONERROR);
        errorOut = L"Missing mobile parameter";
        return false;
    }

    CURL* curl;
    CURLcode res;
    bool result = false;
    
    // Don't call curl_global_init here - it should be called once at app startup
    curl = curl_easy_init();

    if (curl) {
        std::ifstream file(pdfPath, std::ios::binary);
        if (!file) {
            std::wstring msg = L"❌ Failed to open file:\n" + pdfPath;
            MessageBoxW(nullptr, msg.c_str(), L"BillBox Upload", MB_OK | MB_ICONERROR);
            errorOut = msg;
            curl_easy_cleanup(curl);
            return false;
        }

        // Read file content
        std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close(); // Close the file immediately after reading

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/pdf");

        // Get store ID and format mobile header
        std::wstring storeID = ReadStoreIDFromConfig();
        std::string mobileHeader;

        if (!storeID.empty()) {
            // Format: "x-mobile: storeID/mobile"
            std::wstring mobileWithStoreID = storeID + L"/" + mobile;
            mobileHeader = "x-mobile: " + std::string(mobileWithStoreID.begin(), mobileWithStoreID.end());
        } else {
            // Fallback to just mobile number
            mobileHeader = "x-mobile: " + std::string(mobile.begin(), mobile.end());
        }

        std::string filenameHeader = "x-filename: " + std::string(filename.begin(), filename.end());
        headers = curl_slist_append(headers, mobileHeader.c_str());
        headers = curl_slist_append(headers, filenameHeader.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.billbox.co.in/upload-bill");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fileContent.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, fileContent.size());
        
        // Set timeout to prevent hanging
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L); // 30 seconds timeout
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L); // 10 seconds connect timeout

        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
    [](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
        if (!ptr || !userdata) return 0;
        size_t realSize = size * nmemb;
        try {
            reinterpret_cast<std::string*>(userdata)->append(ptr, realSize);
        } catch (...) {
            return 0; // trigger CURLE_WRITE_ERROR
        }
        return realSize;
    }
);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            std::string errorStr = curl_easy_strerror(res);
            std::wstring msg = L"❌ Upload failed: " + std::wstring(errorStr.begin(), errorStr.end());
         //   MessageBoxW(nullptr, msg.c_str(), L"BillBox Upload", MB_OK | MB_ICONERROR);
            errorOut = msg;
        }
        else {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            
            if (response_code >= 200 && response_code < 300) {
                responseOut = std::wstring(response.begin(), response.end());
                result = true;
            } else {
                std::wstring msg = L"❌ Server returned error code: " + std::to_wstring(response_code);
                if (!response.empty()) {
                    msg += L"\nResponse: " + std::wstring(response.begin(), response.end());
                }
                MessageBoxW(nullptr, msg.c_str(), L"BillBox Upload", MB_OK | MB_ICONERROR);
                errorOut = msg;
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    else {
        errorOut = L"Failed to initialize CURL";
        MessageBoxW(nullptr, L"❌ Failed to initialize CURL", L"BillBox Upload", MB_OK | MB_ICONERROR);
    }

    // Don't call curl_global_cleanup here - it should be called once at app shutdown
    return result;
}
