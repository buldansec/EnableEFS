#include <windows.h>
#include "EnableEFS.h"
#include "beacon.h"

BOOL IsEfsServiceRunning() {
    SC_HANDLE scm = ADVAPI32$OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) return FALSE;

    SC_HANDLE service = ADVAPI32$OpenServiceW(scm, L"EFS", SERVICE_QUERY_STATUS);
    if (!service) {
        ADVAPI32$CloseServiceHandle(scm);
        return FALSE;
    }

    SERVICE_STATUS_PROCESS ssp = {0};
    DWORD bytesNeeded = 0;
    BOOL result = ADVAPI32$QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &bytesNeeded);

    ADVAPI32$CloseServiceHandle(service);
    ADVAPI32$CloseServiceHandle(scm);

    return (result && ssp.dwCurrentState == SERVICE_RUNNING);
}

BOOL CreateTempFile(LPWSTR tempFilePath, formatp* buffer) {
    WCHAR tempDir[MAX_PATH];
    BeaconFormatPrintf(buffer, "[*] Getting Temp directory and creating a temp file.\n");
    if (!KERNEL32$GetTempPathW(MAX_PATH, tempDir)) {
        BeaconFormatPrintf(buffer, "  [!] GetTempPathW failed.\n");
        return FALSE;
    }
    BeaconFormatPrintf(buffer, "  [+] Temp directory: %ls\n", tempDir);

    if (!KERNEL32$GetTempFileNameW(tempDir, L"EFS", 0, tempFilePath)) {
        BeaconFormatPrintf(buffer, "  [!] GetTempFileNameW failed.\n");
        return FALSE;
    }
    BeaconFormatPrintf(buffer, "  [+] Created empty temp file: %ls\n", tempFilePath);

    return TRUE;
}

void SuppressEfsPopup(formatp* buffer) {
    HKEY hKey;
    DWORD value = 1;

    if (ADVAPI32$RegCreateKeyExW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows NT\\CurrentVersion\\EFS",
            0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        if (ADVAPI32$RegSetValueExW(hKey, L"EfsUIShown", 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD)) == ERROR_SUCCESS) {
            BeaconFormatPrintf(buffer, "  [+] Registry value EfsUIShown set successfully.\n");
        } else {
            BeaconFormatPrintf(buffer, "  [!] Failed to set EfsUIShown registry value.\n");
        }
        ADVAPI32$RegCloseKey(hKey);
    } else {
        BeaconFormatPrintf(buffer, "  [!] Failed to create/open EFS registry key.\n");
    }
}

void CleanupEfsPopup(formatp* buffer) {
    HKEY hKey;

    if (ADVAPI32$RegOpenKeyExW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows NT\\CurrentVersion\\EFS",
            0, KEY_SET_VALUE,
            &hKey) == ERROR_SUCCESS)
    {
        if (ADVAPI32$RegDeleteValueW(hKey, L"EfsUIShown") == ERROR_SUCCESS) {
            BeaconFormatPrintf(buffer, "  [+] Clean-up: Removed EfsUIShown registry value.\n");
        } else {
            BeaconFormatPrintf(buffer, "  [!] Failed to remove EfsUIShown value.\n");
        }
        ADVAPI32$RegCloseKey(hKey);
    } else {
        BeaconFormatPrintf(buffer, "  [!] Failed to open EFS registry key for cleanup.\n");
    }
}

void go(char* args, int len) {
    formatp buffer;
    BeaconFormatAlloc(&buffer, 4096);

    if (IsEfsServiceRunning()) {
        BeaconFormatPrintf(&buffer, "[*] EFS is already running. No action needed.\n");
        goto cleanup;
    }

    BeaconFormatPrintf(&buffer, "[*] EFS is not running. Attempting to trigger it by encrypting a temp file.\n");
    WCHAR tempFilePath[MAX_PATH];
    if (!CreateTempFile(tempFilePath, &buffer)) {
        BeaconFormatPrintf(&buffer, "[!] Failed to create temp file. Aborting.\n");
        goto cleanup;
    }

    BeaconFormatPrintf(&buffer, "[*] Suppressing EFS GUI popup by setting EfsUIShown in user registry.\n");
    SuppressEfsPopup(&buffer);

    BeaconFormatPrintf(&buffer, "[*] Encrypting temp file.\n");
    if (!ADVAPI32$EncryptFileW(tempFilePath)) {
        BeaconFormatPrintf(&buffer, "[!] EncryptFileW failed.\n");
        KERNEL32$DeleteFileW(tempFilePath);
        goto cleanup;
    }

    BeaconFormatPrintf(&buffer, "  [+] Temp file encrypted successfully.\n");

    BeaconFormatPrintf(&buffer, "[*] Checking the EFS service after the encryption trigger.\n");    
    if (IsEfsServiceRunning()) {
        BeaconFormatPrintf(&buffer, "  [+] EFS service is now RUNNING.\n");
    } else {
        BeaconFormatPrintf(&buffer, "  [!] EFS service still NOT running.\n");
    }

    BeaconFormatPrintf(&buffer, "[*] Proceeding with clean-up.\n");
    if (KERNEL32$DeleteFileW(tempFilePath)) {
        BeaconFormatPrintf(&buffer, "  [+] Clean-up: Temp file deleted.\n");
    } else {
        BeaconFormatPrintf(&buffer, "  [!] DeleteFileW failed.\n");
    }

    CleanupEfsPopup(&buffer);
    BeaconFormatPrintf(&buffer, "[+] Done.\n");

    cleanup:
    BeaconPrintf(CALLBACK_OUTPUT, "%s", BeaconFormatToString(&buffer, NULL));
    BeaconFormatFree(&buffer);
}
