#include <windows.h>
#include <stdio.h>

#include "nocrt.h"

/*
	https://learn.microsoft.com/en-us/windows/win32/sysinfo/deleting-a-key-with-subkeys
*/

//*************************************************************
//
//  RegDelnodeRecurse()
//
//  Purpose:    Deletes a registry key and all its subkeys / values.
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
//
//  Return:     TRUE if successful.
//              FALSE if an error occurs.
//
//*************************************************************

BOOL RegDelnodeRecurse (HKEY hKeyRoot, LPTSTR lpSubKey)
{
	LPTSTR lpEnd;
	LONG lResult;
	DWORD dwSize;
	char szName[MAX_PATH];
	HKEY hKey;
	FILETIME ftWrite;

	// First, see if we can delete the key without having
	// to recurse.

	lResult = RegDeleteKey(hKeyRoot, lpSubKey);

	if(lResult == ERROR_SUCCESS) 
		return TRUE;

	lResult = RegOpenKeyEx(hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);

	if(lResult != ERROR_SUCCESS) 
	{
		if (lResult == ERROR_FILE_NOT_FOUND)
		{
			// printf("Key not found.\n");
			return TRUE;
		} 
		else
		{
			// printf("Error opening key.\n");
			return FALSE;
		}
	}

	// Check for an ending slash and add one if it is missing.

	lpEnd = lpSubKey + lstrlen(lpSubKey);

	if(*(lpEnd - 1) != TEXT('\\')) 
	{
		*lpEnd =  TEXT('\\');
		lpEnd++;
		*lpEnd =  TEXT('\0');
	}

	// Enumerate the keys

	dwSize = MAX_PATH;
	lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL, NULL, NULL, &ftWrite);

	if (lResult == ERROR_SUCCESS) 
	{
		do
		{
			*lpEnd = TEXT('\0');
			strcat(lpSubKey, szName);

			if(!RegDelnodeRecurse(hKeyRoot, lpSubKey)) break;

			dwSize = MAX_PATH;

			lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL, NULL, NULL, &ftWrite);
		}while (lResult == ERROR_SUCCESS);
	}

	lpEnd--;
	*lpEnd = TEXT('\0');

	RegCloseKey (hKey);

	// Try again to delete the key.

	lResult = RegDeleteKey(hKeyRoot, lpSubKey);

	if(lResult == ERROR_SUCCESS) 
		return TRUE;

	return FALSE;
}

//*************************************************************
//
//  RegDelnode()
//
//  Purpose:    Deletes a registry key and all its subkeys / values.
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
//
//  Return:     TRUE if successful.
//              FALSE if an error occurs.
//
//*************************************************************

BOOL RegDelnode(HKEY hKeyRoot, LPCTSTR lpSubKey)
{
	char szDelKey[MAX_PATH*2];
	strcpy(szDelKey, lpSubKey);
	return RegDelnodeRecurse(hKeyRoot, szDelKey);
}
