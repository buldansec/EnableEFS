// Author: buldansec
// Version: 1.0
// Last update: 06.07.2025

#include <windows.h>
#include <winsvc.h>
#include <stdio.h>
#include <shlwapi.h>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Shlwapi.lib")

void PrintLastError(const wchar_t* context) {
    DWORD err = GetLastError();
    LPWSTR msg = NULL;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, err, 0, (LPWSTR)&msg, 0, NULL);
    wprintf(L"[!] %s failed. Error %lu: %s\n", context, err, msg);
    LocalFree(msg);
}

LPCWSTR SspStateToString(DWORD state) {
    switch (state) {
        case SERVICE_STOPPED: return L"SERVICE_STOPPED";
        case SERVICE_START_PENDING: return L"SERVICE_START_PENDING";
        case SERVICE_STOP_PENDING: return L"SERVICE_STOP_PENDING";
        case SERVICE_RUNNING: return L"SERVICE_RUNNING";
        case SERVICE_CONTINUE_PENDING: return L"SERVICE_CONTINUE_PENDING";
        case SERVICE_PAUSE_PENDING: return L"SERVICE_PAUSE_PENDING";
        case SERVICE_PAUSED: return L"SERVICE_PAUSED";
        default: return L"UNKNOWN_STATE";
    }
}

BOOL IsEfsServiceRunning() {
    SERVICE_STATUS_PROCESS ssp;
    DWORD bytesNeeded = 0;

    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) {
        PrintLastError(L"OpenSCManager");
        return FALSE;
    }

    SC_HANDLE service = OpenServiceW(scm, L"EFS", SERVICE_QUERY_STATUS);
    if (!service) {
        PrintLastError(L"OpenService");
        CloseServiceHandle(scm);
        return FALSE;
    }

    BOOL result = QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO,
        (LPBYTE)&ssp, sizeof(ssp), &bytesNeeded);

    if (!result) {
        PrintLastError(L"QueryServiceStatusEx");
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return FALSE;
    }

    wprintf(L"[!] EFS current state: %s (%lu)\n", SspStateToString(ssp.dwCurrentState), ssp.dwCurrentState);

    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    return (ssp.dwCurrentState == SERVICE_RUNNING);
}

BOOL CreateTempFile(LPWSTR tempFilePath) {
    WCHAR tempDir[MAX_PATH];

    if (!GetTempPathW(MAX_PATH, tempDir)) {
        PrintLastError(L"GetTempPathW");
        return FALSE;
    }
    wprintf(L"  [+] Temp directory: %ls\n", tempDir);

    if (!GetTempFileNameW(tempDir, L"EFS", 0, tempFilePath)) {
        PrintLastError(L"GetTempFileNameW");
        return FALSE;
    }
    wprintf(L"  [+] Created empty temp file: %ls\n", tempFilePath);

    return TRUE;
}

void SuppressEfsPopup() {
    HKEY hKey;
    DWORD value = 1;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\EFS",
        0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueExW(hKey, L"EfsUIShown", 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD));
        RegCloseKey(hKey);
        wprintf(L"  [+] EFS popup suppressed (EfsUIShown=1).\n");
    } else {
        PrintLastError(L"RegCreateKeyExW");
    }
}

void CleanupEfsPopup() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\EFS",
        0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
    {
        if (RegDeleteValueW(hKey, L"EfsUIShown") == ERROR_SUCCESS) {
            wprintf(L"  [+] Clean-up: Removed EfsUIShown registry value.\n");
        } else {
            PrintLastError(L"RegDeleteValueW");
        }
        RegCloseKey(hKey);
    } else {
        PrintLastError(L"RegOpenKeyExW");
    }
}


int wmain() {
    wprintf(L"\n[+] Starting EnableEFS Tool by buldansec\n");
    wprintf(L"====================================\n");

    if (IsEfsServiceRunning()) {
        wprintf(L"[+] EFS is already running. No action needed.\n");
        return 0;
    }

    wprintf(L"[!] EFS is not running. Attempting to trigger it by encrypting a temp file.\n");

    wprintf(L"[*] Getting Temp directory and creating a temp file.\n");
    WCHAR tempFilePath[MAX_PATH];
    if (!CreateTempFile(tempFilePath)) {
        wprintf(L"[!] Failed to create temp file. Aborting.\n");
        return 1;
    }
    wprintf(L"[*] Suppressing EFS GUI popup by setting EfsUIShown in user registry.\n");
    SuppressEfsPopup();

    wprintf(L"[*] Encrypting temp file.\n");
    if (!EncryptFileW(tempFilePath)) {
        PrintLastError(L"EncryptFileW");
        DeleteFileW(tempFilePath);
        return 1;
    }
    wprintf(L"  [+] Temp file encrypted successfully.\n");

    wprintf(L"[*] Checking the EFS service after the encryption trigger.\n");
    if (IsEfsServiceRunning()) {
        wprintf(L"  [+] EFS service is now RUNNING.\n");
    } else {
        wprintf(L"  [!] EFS service still NOT running.\n");
    }

    wprintf(L"[*] Proceeding with clean-up.\n");
    
    if (DeleteFileW(tempFilePath)) {
        wprintf(L"  [+] Clean-up: Temp file deleted.\n");
    } else {
        PrintLastError(L"DeleteFileW");
    }

    CleanupEfsPopup();

    wprintf(L"[+] Done.\n");
    return 0;
}
