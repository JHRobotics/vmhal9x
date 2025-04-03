/******************************************************************************
 * Copyright (c) 2025 Jaroslav Hensl                                          *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person                *
 * obtaining a copy of this software and associated documentation             *
 * files (the "Software"), to deal in the Software without                    *
 * restriction, including without limitation the rights to use,               *
 * copy, modify, merge, publish, distribute, sublicense, and/or sell          *
 * copies of the Software, and to permit persons to whom the                  *
 * Software is furnished to do so, subject to the following                   *
 * conditions:                                                                *
 *                                                                            *
 * The above copyright notice and this permission notice shall be             *
 * included in all copies or substantial portions of the Software.            *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,            *
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES            *
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND                   *
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT                *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,               *
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING               *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR              *
 * OTHER DEALINGS IN THE SOFTWARE.                                            *
 *                                                                            *
 ******************************************************************************/

/****
	New storage (all HKLM):

	SOFTWARE\\vmdisp9x\\apps\\global\\
	SOFTWARE\\vmdisp9x\\apps\\exe\\<exe name>\\
	SOFTWARE\\vmdisp9x\\apps\\profile\\<profile name>\\

	path - string, regex of path
	name - string, profile name like application or game full namne
	desc - string, description for example why profile exists
	
	wine\\ wine settings
	mesa\\ mesa settings
	hal\\ vmhal settings
	
	ddraw - string, ddraw replacement
	d3d8 - string, DX8 replacement
	d3d9 - string, DX9 replacement

	Compatibility:
	SOFTWARE\\DDSwitcher (name "exe", value "libname")
	SOFTWARE\\D8Switcher
	SOFTWARE\\D9Switcher
	
	SOFTWARE\\Wine\\<exe or global>
	Software\\Mesa3D\\<exe or global>
	
	Another keys:
	SOFTWARE\\vmdisp9x\\driver (global driver settings)
	SOFTWARE\\vmdisp9x\\svga (VMWare SVGA settings)

****/

#include <windows.h>
#include <stdio.h>
#include <string.h>

#define VMHAL9X_LIB
#include "vmsetup.h"

#include "re.h"

#include "nocrt.h"

#define DATA_NULL   0
#define DATA_STR    1
#define DATA_DWORD  2

#define SETUP_MAXSIZE 8192

#define REG_DATA_MAX 254

typedef struct _settings_block_t
{
	char *name;
	char *category;
	char *data_str;
	DWORD data_dw;
	struct _settings_block_t *next;
	char *next_data;
	DWORD data_left;
} settings_block_t;

static char setup[SETUP_MAXSIZE];

static settings_block_t first_block = {
	NULL,
	NULL,
	NULL,
	DATA_NULL,
	NULL,
	setup,
	SETUP_MAXSIZE
};

static settings_block_t *last_block = &first_block;

static BOOL alloc_block(const char *name,	const char *category,	void *data,	DWORD data_type)
{
	size_t size_name = 0; 
	size_t size_category = 0; 
	size_t size_data_str = 0;
	
	char *data_str = NULL;
	DWORD data_dw;
	char strbuf[16];

	if(name != NULL) size_name = strlen(name)+1;

	if(category != NULL) size_category = strlen(category)+1;

	if(data != NULL)
	{
		if(data_type == DATA_STR)
		{
			data_str = (char*)data;
			size_data_str = strlen(data_str)+1;
			data_dw = strtoul(data_str, NULL, 0);
		}
		else if(data_type == DATA_DWORD)
		{
			data_dw = *((DWORD*)data);
			data_str = strbuf;
			sprintf(data_str, "%lu", data_dw);
			size_data_str = strlen(data_str)+1;
		}
		else
		{
			data_str = NULL;
			data_dw = 0;
			size_data_str = 0;
		}
	}
	else
	{
		data_str = NULL;
		size_data_str = 0;
		data_dw = 0;
	}
	
	size_t size_total = sizeof(settings_block_t) +  size_name + size_category + size_data_str;
	
	if(last_block->data_left > size_total)
	{
		char *ptr = last_block->next_data;
		settings_block_t *block = (settings_block_t*)ptr;
		ptr += sizeof(settings_block_t);

		if(name != NULL)
		{
			block->name = ptr;
			memcpy(block->name, name, size_name);

			ptr += size_name;
		}
		else
		{
			block->name = NULL;
		}

		if(category != NULL)
		{
			block->category = ptr;
			memcpy(block->category, category, size_category);
			
			ptr += size_category;
		}
		else
		{
			block->category = NULL;
		}

		block->data_dw   = data_dw;
		if(size_data_str > 0)
		{
			block->data_str = ptr;
			memcpy(block->data_str, data_str, size_data_str);
			ptr += size_data_str;
		}
		else
		{
			block->data_str = NULL;
		}

		block->next_data = ptr;
		block->next = NULL;
		block->data_left = last_block->data_left - size_total;

		last_block->next = block;
		last_block = block;

		return TRUE;
	}
	return FALSE;	
}

static settings_block_t *vmhal_settings_block(const char *category, const char *name)
{
	settings_block_t *block = &first_block;
	
	while(block != NULL)
	{
		if(category == NULL)
		{
			if(block->category == NULL)
			{
				if(block->name != NULL && stricmp(block->name, name) == 0)
				{
					return block;
				}
			}
		}
		else
		{
			if(block->category != NULL && stricmp(block->category, category) == 0)
			{
				if(block->name != NULL && stricmp(block->name, name) == 0)
				{
					return block;
				}
			}
		}
		
		block = block->next;
	}
	
	return NULL;
}

const char *vmhal_setup_str(const char *category, const char *name, BOOL empty_str)
{
	settings_block_t *block = vmhal_settings_block(category, name);
	if(block)
	{
		if(block->data_str != NULL)
		{
			return block->data_str;
		}
	}

	if(empty_str)
	{
		return "";
	}

	return NULL;
}

DWORD vmhal_setup_dw(const char *category, const char *name)
{
	settings_block_t *block = vmhal_settings_block(category, name);
	if(block)
	{
		if(block->data_str != NULL)
		{
			return block->data_dw;
		}
	}
	return 0;
}

static void reg_lookup(HKEY hKey, const char *category)
{
	char name_buf[REG_DATA_MAX+1];
	char data_buf[REG_DATA_MAX+1];
	
	DWORD index = 0;
	LSTATUS s;
	
	do
	{
		DWORD name_size = REG_DATA_MAX;
		DWORD data_size = REG_DATA_MAX;
		DWORD type;

		s = RegEnumValueA(hKey, index, name_buf, &name_size, NULL, &type, (LPBYTE)data_buf, &data_size);
		if(s == ERROR_SUCCESS)
		{
			switch(type)
			{
				case REG_DWORD:
					alloc_block(name_buf,	category,	data_buf,	DATA_DWORD);
					break;
				case REG_SZ:
					alloc_block(name_buf,	category,	data_buf,	DATA_STR);
					break;
			}
		}

		index++;
	} while(s != ERROR_NO_MORE_ITEMS);	
}

static const char *categories[] = {
	"wine",
	"mesa",
	"hal",
	"openglide",
	NULL
};

static const char *ddswitcher[][2] = {
	{"SOFTWARE\\DDSwitcher", "ddraw"},
	{"SOFTWARE\\D8Switcher", "d3d8"},
	{"SOFTWARE\\D9Switcher", "d3d9"},
	{NULL, NULL}
};

static const char *categories_compact[][2] = {
	{"SOFTWARE\\Wine",      "wine"},
	{"SOFTWARE\\Mesa3D",    "mesa"},
	{"SOFTWARE\\OpenGlide", "openglide"},
	{NULL, NULL}
};

static void reg_profile(HKEY hKey)
{
	HKEY cat_key;

	reg_lookup(hKey, NULL);
	int c = 0;
	
	while(categories[c] != NULL)
	{
		if(RegOpenKeyExA(hKey, categories[c], 0, KEY_READ, &cat_key) == ERROR_SUCCESS)
		{
			reg_lookup(cat_key, categories[c]);
			RegCloseKey(cat_key);
		}
		c++;
	}
}

void vmhal_setup_load(BOOL compat)
{
	char path[PATH_MAX];
	char *exe;
	size_t path_size;
	if((path_size = GetModuleFileName(NULL, path, PATH_MAX)) == 0)
	{
		return;
	}

	int i;
	for(i = 0; i < path_size; i++)
	{
		if(path[i] == '\\')
			path[i] = '/';
		else
			path[i] = tolower(path[i]);
	}

	exe = strrchr(path, '/');
	if(exe == NULL)
	{
		exe = path;
	}
	else
	{
		exe++;
	}
	
	//printf("exe = %s, path = %s\n", exe, path);

	char data[REG_DATA_MAX+1];
	char regex_path[REG_DATA_MAX+1];

	HKEY profiles;
	if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\vmdisp9x\\apps\\profile", 0, KEY_READ, &profiles) == ERROR_SUCCESS)
	{
		DWORD index = 0;
		LSTATUS s;
		do
		{
			DWORD data_size = REG_DATA_MAX;
			s = RegEnumKeyExA(profiles, index, data, &data_size, NULL, NULL, NULL, NULL);
			if(s == ERROR_SUCCESS)
			{
				HKEY profile;
				if(RegOpenKeyExA(profiles, data, 0, KEY_READ, &profile) == ERROR_SUCCESS)
				{
					DWORD regex_path_size = REG_DATA_MAX;
					DWORD type;
					if(RegQueryValueExA(profile, "path", NULL, &type, (LPBYTE)regex_path, &regex_path_size) == ERROR_SUCCESS)
					{
						if(type == REG_SZ)
						{
							int ml;
							if(re_match(regex_path, path, &ml) > 0)
							{
								//printf("reg match: %s %s (%d)\n", regex_path, path, ml);
								reg_profile(profile);
							}
						}
					}
					RegCloseKey(profile);
				}
			}
			index++;
		} while(s != ERROR_NO_MORE_ITEMS);	
		RegCloseKey(profiles);
	}

	if(strlen(exe) + sizeof("SOFTWARE\\vmdisp9x\\apps\\exe\\%s") <= sizeof(regex_path))
	{
		sprintf(regex_path, "SOFTWARE\\vmdisp9x\\apps\\exe\\%s", exe);
		HKEY exe_key;
		if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, regex_path, 0, KEY_READ, &exe_key) == ERROR_SUCCESS)
		{
			reg_profile(exe_key);
			RegCloseKey(exe_key);
		}
	}
	
	HKEY global_profile;
	if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\vmdisp9x\\apps\\global", 0, KEY_READ, &global_profile) == ERROR_SUCCESS)
	{
		reg_profile(global_profile);
		RegCloseKey(global_profile);
	}
	
	if(compat)
	{
		/* read DDswitcher settings */
		for(i = 0; ddswitcher[i][0] != NULL; i++)
		{
			HKEY switcher;
			if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, ddswitcher[i][0], 0, KEY_READ, &switcher) == ERROR_SUCCESS)
			{
				DWORD data_size = REG_DATA_MAX;
				DWORD type;
				if(RegQueryValueExA(switcher, exe, NULL, &type, (LPBYTE)data, &data_size) == ERROR_SUCCESS)
				{
					switch(type)
					{
						case REG_SZ:
							alloc_block(ddswitcher[i][1],	NULL,	data, DATA_STR);
							break;
						case REG_DWORD:
							alloc_block(ddswitcher[i][1],	NULL,	data, DATA_DWORD);
							break;
					}
				}
				RegCloseKey(switcher);
			}
		} // for ddswitcher

		for(i = 0; categories_compact[i][0] != NULL; i++)
		{
			HKEY hk_cat_top;
			if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, categories_compact[i][0], 0, KEY_READ, &hk_cat_top) == ERROR_SUCCESS)
			{
				HKEY hk_exe;
				if(RegOpenKeyExA(hk_cat_top, exe, 0, KEY_READ, &hk_exe) == ERROR_SUCCESS)
				{
					reg_lookup(hk_exe, categories_compact[i][1]);
					RegCloseKey(hk_exe);
				}
				if(RegOpenKeyExA(hk_cat_top, "global", 0, KEY_READ, &hk_exe) == ERROR_SUCCESS)
				{
					reg_lookup(hk_exe, categories_compact[i][1]);
					RegCloseKey(hk_exe);
				}
				RegCloseKey(hk_cat_top);
			}
		} // for categories_compact
	} // compat
}
