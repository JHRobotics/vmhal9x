#include <initguid.h>
#include <windows.h>
#include <wingdi.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ddraw.h>

#include "../3d_accel.h"

#define VERSION_ONLY
#include "../vmhal9x.h"

#include "nocrt.h"

void WaitEnter()
{
	int c;
	printf("Press Return to exit");
	do
	{
		c = getchar();
	} while(!(c == '\n' || c == EOF));
}

#define RT_PRINT_HELP 0
#define RT_PRINT_LIST 1
#define RT_INSER_MODES 2

void print_list()
{
	AllocConsole();
	FBHDA_t *hda = FBHDA_setup();

	if(hda)
	{
		if(hda->flags & FB_VESA_MODES)
		{
			int mode_index = 0;
			FBHDA_mode_t mode;
			mode.cb = sizeof(FBHDA_mode_t);
			
			printf("Looping modes:\n");
			
			while(FBHDA_mode_query(mode_index, &mode))
			{
				printf("mode %d: %d x %d x %d\n", mode_index, mode.width, mode.height, mode.bpp);
				mode_index++;
			}
		}
		else
		{
			printf("This works only for VESA driver!\n");
		}
	}
	else
	{
		printf("Cannot connect to driver!\n");
	}

	WaitEnter();
}

#define DISPLAY_BASE_KEY "System\\CurrentControlSet\\Services\\Class\\Display"

BOOL find_base_reg(char *outbuf)
{
	HKEY key;
	FILETIME ftWrite;
	char szName[MAX_PATH];
	char szTestBuf[MAX_PATH];
	char szSectionName[MAX_PATH];
	LSTATUS lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, DISPLAY_BASE_KEY, 0, KEY_READ, &key);
	BOOL rc = FALSE;

	if(lResult == ERROR_SUCCESS) 
	{
		DWORD index = 0;
		do
		{
			DWORD dwSize = MAX_PATH;
			lResult = RegEnumKeyEx(key, index, szName, &dwSize, NULL, NULL, NULL, &ftWrite);
			if(lResult == ERROR_SUCCESS)
			{
				sprintf(szTestBuf, "%s\\%s", DISPLAY_BASE_KEY, szName);
				
				HKEY key2;
				LSTATUS lResult2 = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTestBuf, 0, KEY_READ, &key2);
				if(lResult2 == ERROR_SUCCESS)
				{
					DWORD buf_s = MAX_PATH;
					RegQueryValueExA(key2, "InfSection", NULL, NULL, (LPBYTE)szSectionName, &buf_s);
					if(strcmp(szSectionName, "VESA") == 0)
					{
						strcpy(outbuf, szTestBuf);
						rc = TRUE;
					}
					RegCloseKey(key2);
				}
				index++;
			}
		} while(lResult == ERROR_SUCCESS);
		RegCloseKey(key);
	}
	return rc;
}

void RegCreateKeyDefault(HKEY root, const char *keyname, const char *subkeyname, const char *defvalue)
{
	HKEY key;
	HKEY subkey;
	
	if(RegOpenKeyEx(root, keyname, 0, KEY_WRITE, &key) == ERROR_SUCCESS)
	{
		if(RegCreateKeyExA(key, subkeyname, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &subkey, NULL) == ERROR_SUCCESS)
		{
			if(defvalue != NULL)
			{
				RegSetValueExA(subkey, NULL, 0, REG_SZ, (LPBYTE)defvalue, strlen(defvalue)+1);
			}
			RegCloseKey(subkey);
		}
		RegCloseKey(key);
	}
}

void RegCreateValue(HKEY root, char *keyname, const char *szName, const char *szData)
{
	HKEY key;

	if(RegOpenKeyEx(root, keyname, 0, KEY_WRITE, &key) == ERROR_SUCCESS)
	{
		RegSetValueExA(key, szName, 0, REG_SZ, (LPBYTE)szData, strlen(szData)+1);
		RegCloseKey(key);
	}
}

BOOL RegDelnode(HKEY hKeyRoot, LPCTSTR lpSubKey);

void insert_modes(BOOL allow_15bpp, BOOL allow_24bpp)
{
	FBHDA_t *hda = FBHDA_setup();
	DWORD count_modes = 0;
	FBHDA_mode_t mode;
	DWORD insert_modes = 0;
	char msgbuf[1024];

	mode.cb = sizeof(FBHDA_mode_t);

	if(hda)
	{
		if(hda->flags & FB_VESA_MODES)
		{
			int mode_index = 0;

			printf("Looping modes:\n");

			while(FBHDA_mode_query(mode_index, &mode))
			{
				printf("mode %d: %d x %d x %d\n", mode_index, mode.width, mode.height, mode.bpp);
				if(mode.width >= 640 && mode.height >= 480)
				{
					switch(mode.bpp)
					{
						case 24:
							if(allow_24bpp)
								count_modes++;
							break;
						case 15:
							if(allow_24bpp)
								count_modes++;
							break;
						case 8:
						case 16:
						case 32:
							count_modes++;
							break;
						default:
							break;
					}
				}
				mode_index++;
			}
		}
	}
	
	if(count_modes)
	{
		char display_base[MAX_PATH];
		char pathbuf[MAX_PATH];
		char pathbuf2[MAX_PATH];

		sprintf(msgbuf, "Video card report %d usable modes, do you want insert them to registry?", count_modes);
		
		if(MessageBoxA(NULL, msgbuf, "Resolution update", MB_YESNO | MB_ICONQUESTION) != IDYES)
		{
			return;
		}

		if(find_base_reg(display_base))
		{
			sprintf(pathbuf, "%s\\%s", display_base, "MODES");
			RegDelnode(HKEY_LOCAL_MACHINE, pathbuf);
			RegCreateKeyDefault(HKEY_LOCAL_MACHINE, display_base, "MODES", NULL);

			strcat(display_base, "\\MODES");

			RegCreateKeyDefault(HKEY_LOCAL_MACHINE, display_base, "8",  NULL);
			RegCreateKeyDefault(HKEY_LOCAL_MACHINE, display_base, "16", NULL);
			RegCreateKeyDefault(HKEY_LOCAL_MACHINE, display_base, "32", NULL);

			if(allow_15bpp)
			{
				RegCreateKeyDefault(HKEY_LOCAL_MACHINE, display_base, "15", NULL);
			}

			if(allow_24bpp)
			{
				RegCreateKeyDefault(HKEY_LOCAL_MACHINE, display_base, "24", NULL);
			}

			int mode_index = 0;
			while(FBHDA_mode_query(mode_index, &mode))
			{
				BOOL insert = FALSE;
				if(mode.width >= 640 && mode.height >= 480)
				{
					switch(mode.bpp)
					{
						case 24:
							if(allow_24bpp)
								insert = TRUE;
							break;
						case 15:
							if(allow_24bpp)
								insert = TRUE;
							break;
						case 8:
						case 16:
						case 32:
							insert = TRUE;
							break;
						default:
							break;
					}
				}
				
				if(insert)
				{
					sprintf(pathbuf, "%s\\%d", display_base, mode.bpp);
					sprintf(pathbuf2, "%d,%d", mode.width, mode.height);
					RegCreateKeyDefault(HKEY_LOCAL_MACHINE, pathbuf, pathbuf2, "");
					insert_modes++;
				}
				mode_index++;
			}

			// write VGA/SVGA bpp
			RegCreateKeyDefault(HKEY_LOCAL_MACHINE, display_base, "4", NULL);
			sprintf(pathbuf, "%s\\4", display_base);
			RegCreateKeyDefault(HKEY_LOCAL_MACHINE, pathbuf, "640,480", NULL);
			sprintf(pathbuf, "%s\\4\\640,480", display_base);
			RegCreateValue(HKEY_LOCAL_MACHINE, pathbuf, "drv", "vga.drv");
			RegCreateValue(HKEY_LOCAL_MACHINE, pathbuf, "vdd", "*vdd");
			sprintf(pathbuf, "%s\\4", display_base);
			RegCreateKeyDefault(HKEY_LOCAL_MACHINE, pathbuf, "800,600", NULL);
			sprintf(pathbuf, "%s\\4\\800,600", display_base);
			RegCreateValue(HKEY_LOCAL_MACHINE, pathbuf, "drv", "supervga.drv");
			RegCreateValue(HKEY_LOCAL_MACHINE, pathbuf, "vdd", "*vdd");
		}
		
		sprintf(msgbuf, "Inserted %d VESA modes + 2 SVGA modes. You should see the new modes in the settings, if not, please restart the computer.", insert_modes);
		
		MessageBoxA(NULL, msgbuf, "Resolution update", MB_OK);
	}
	else
	{
		MessageBoxA(NULL, "No VESA modes found", "Resolution update", MB_OK);
	}
}

int main(int argc, char **argv)
{
	int rt = RT_PRINT_HELP;
	BOOL bpp15 = FALSE;
	BOOL bpp24 = FALSE;
	int i;
	for(i = 1; i < argc; i++)
	{
		if(stricmp(argv[i], "/list") == 0)
		{
			rt = RT_PRINT_LIST;
		}

		if(stricmp(argv[i], "/insert") == 0)
		{
			rt = RT_INSER_MODES;
		}
		
		if(strcmp(argv[i], "/15") == 0)
		{
			bpp15 = TRUE;
		}

		if(strcmp(argv[i], "/24") == 0)
		{
			bpp24 = TRUE;
		}
	}

	switch(rt)
	{
		case RT_PRINT_HELP:
			MessageBoxA(NULL,
				"Print modes:\nvesamode /list\n\n"
				"Update modes from VESA BIOS:\nvesamode /insert [/15] [/24]\n"
				"  /15: insert 15 bpp modes\n"
				"  /24: insert 24 bpp modes\n",
				"Usage", MB_ICONINFORMATION);
			break;
		case RT_PRINT_LIST:
			print_list();
			break;
		case RT_INSER_MODES:
			insert_modes(bpp15, bpp24);
			break;
	}
	
	return EXIT_SUCCESS;
}
