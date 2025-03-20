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
#include <windows.h>
#include <stddef.h>
#include <stdint.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>

#include <tlhelp32.h>

#include "vmdahal32.h"
#include "vmhal9x.h"

#include "hotpatch.h"

#include "nocrt.h"

#ifndef P_SIZE
#define P_SIZE 4096
#endif

static int codepos = 0;
static const BYTE patchablecode[] = {0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x8B, 0xFF};
static BYTE codejmp[P_SIZE] __attribute__ ((aligned (P_SIZE))) = {0};

#define PATCHABLE_POS (-5)
#define RETURN_POS 2
#define PATCH_SIZE sizeof(patchablecode)

BOOL patch_init()
{
	DWORD old;
	return VirtualProtect(&codejmp[0], P_SIZE, PAGE_EXECUTE_READWRITE, &old);
}

#define WRITE_OP_1B(_b) codejmp[codepos] = _b; codepos++;
#define WRITE_OP_DW(_dw) *((DWORD*)&codejmp[codepos]) = _dw; codepos += sizeof(DWORD);
#define RELATIVE_ADDR(_adr) (((DWORD)(_adr)) - ((DWORD)&codejmp[codepos+4]))

#define WRITE_OP_2B(_b1, _b2) WRITE_OP_1B(_b1) WRITE_OP_1B(_b2)
#define WRITE_OP_3B(_b1, _b2, _b3) WRITE_OP_2B(_b1, _b2) WRITE_OP_1B(_b3)
#define WRITE_OP_4B(_b1, _b2, _b3, _b4) WRITE_OP_2B(_b1, _b2) WRITE_OP_2B(_b3, _b4)

void *patch_make_trampoline(const char *libname, const char *procname, patchedAddProc_h proc)
{
	void *patch_code = &codejmp[codepos];
	
	//WRITE_OP_4B(0x8b, 0x44, 0x24, 0x04)  // mov eax, [esp+4]   4

	WRITE_OP_2B(0x89, 0xE0)              // mov  eax, esp      2
	WRITE_OP_3B(0x83, 0xC0, 0x04)        // add  eax, 0x4      5
	WRITE_OP_1B(0x50)                    // push eax           6
	
	WRITE_OP_1B(0x68)                    // push               6
	WRITE_OP_DW((DWORD)procname)         // arg procname      10
	WRITE_OP_1B(0x68)                    // push              11
	WRITE_OP_DW((DWORD)libname)          // arg libname       15
	WRITE_OP_1B(0xE8)                    // call              16
	WRITE_OP_DW(RELATIVE_ADDR(proc))     // rel proc          20
	WRITE_OP_2B(0x85, 0xC0)              // test eax, eax     22
	WRITE_OP_2B(0x74, 0x05)              // jz +5             24
	WRITE_OP_3B(0x83, 0xc4, 0x04)        // add esp, 4        27
	WRITE_OP_2B(0xFF, 0xE0)              // jmp eax           29
	WRITE_OP_1B(0x58)                    // pop eax           30
	WRITE_OP_3B(0x83, 0xC0, RETURN_POS)  // add eax, 2        33
	WRITE_OP_2B(0xff, 0xE0)              // jmp eax           35
	//WRITE_OP_1B(0xCC)                    // align             36

	HANDLE hProc = GetCurrentProcess();
	FlushInstructionCache(hProc, &codejmp, P_SIZE);
	
	return patch_code;
}

#define BUILD_PATCH(_buf, _trampoline, _original) \
	(_buf)[0] = 0xE8; /* call */ \
	*((DWORD*)&(_buf)[1]) = ((DWORD)_trampoline) - ((DWORD)_original); /* (+ PATCHABLE_POS + 5) */ \
	(_buf)[5] = 0xEB; /* jmp near */ \
	(_buf)[6] = 0xF9; /* -7 */

static BOOL patch_install_address(BYTE *original, void *trampoline)
{
	BOOL rc = FALSE;
	
	if(trampoline == NULL)
	{
		TOPIC("PATCH", "trampoline is NULL");
		return FALSE;
	}

	if(memcmp(original+PATCHABLE_POS, patchablecode, PATCH_SIZE) == 0)
	{
		BYTE patch[PATCH_SIZE];

		BUILD_PATCH(patch, trampoline, original);

		if((DWORD)original >= 0x80000000)
		{
			TOPIC("PATCH", "patch shared=0x%X", original);
			FBHDA_page_modify((DWORD)(original+PATCHABLE_POS), PATCH_SIZE, patch);
			rc = TRUE;
		}
		else
		{
			TOPIC("PATCH", "patch local=0x%X", original);
			DWORD oldp;
			if(VirtualProtect(original+PATCHABLE_POS, PATCH_SIZE, PAGE_EXECUTE_READWRITE, &oldp))
			{
				memcpy(original+PATCHABLE_POS, &patch[0], PATCH_SIZE);
				rc = TRUE;
				VirtualProtect(original+PATCHABLE_POS, PATCH_SIZE, oldp, &oldp);
			}
		}

		if(rc)
		{
			HANDLE hProc = GetCurrentProcess();
			FlushInstructionCache(hProc, original+PATCHABLE_POS, 0);
		}
		return rc;
	}

	return FALSE;
}

static BOOL patch_uninstall_address(BYTE *original, void *trampoline)
{
	BOOL rc = FALSE;
	
	if(trampoline == NULL)
	{
		TOPIC("PATCH", "trampoline is NULL");
		return FALSE;
	}
	
	BYTE patch[PATCH_SIZE];
	BUILD_PATCH(patch, trampoline, original);

	if(memcmp(original+PATCHABLE_POS, patch, PATCH_SIZE) == 0)
	{
		if((DWORD)original >= 0x80000000)
		{
			TOPIC("PATCH", "patch shared=0x%X", original);
			FBHDA_page_modify((DWORD)(original+PATCHABLE_POS), PATCH_SIZE, patchablecode);
			rc = TRUE;
		}
		else
		{
			TOPIC("PATCH", "patch local=0x%X", original);
			DWORD oldp;
			if(VirtualProtect(original+PATCHABLE_POS, PATCH_SIZE, PAGE_EXECUTE_READWRITE, &oldp))
			{
				memcpy(original+PATCHABLE_POS, &patchablecode[0], PATCH_SIZE);
				rc = TRUE;
				VirtualProtect(original+PATCHABLE_POS, PATCH_SIZE, oldp, &oldp);
			}
		}

		if(rc)
		{
			HANDLE hProc = GetCurrentProcess();
			FlushInstructionCache(hProc, original+PATCHABLE_POS, 0);
		}
		return rc;
	}

	return FALSE;
}

static int patch_validate_address(BYTE *original, void *trampoline)
{
	if(memcmp(original+PATCHABLE_POS, patchablecode, PATCH_SIZE) == 0)
	{
		return PATCH_PATCHABLE;
	}

	if(trampoline != NULL)
	{
		BYTE patch[PATCH_SIZE];
		BUILD_PATCH(patch, trampoline, original);
		
		if(memcmp(original+PATCHABLE_POS, patch, PATCH_SIZE) == 0)
		{
			return PATCH_PATCHED;
		}
	}

	return PATCH_NOTAPPLICABLE;
}

#pragma pack(push)
#pragma pack(1)

/* http://www.delorie.com/djgpp/doc/exe/ */
typedef struct EXE_header {
  uint16_t signature; /* == 0x5a4D */
  uint16_t bytes_in_last_block;
  uint16_t blocks_in_file;
  uint16_t num_relocs;
  uint16_t header_paragraphs;
  uint16_t min_extra_paragraphs;
  uint16_t max_extra_paragraphs;
  uint16_t ss;
  uint16_t sp;
  uint16_t checksum;
  uint16_t ip;
  uint16_t cs;
  uint16_t reloc_table_offset;
  uint16_t overlay_number;
	uint16_t e_res[4];
	uint16_t e_oemid;
	uint16_t e_oeminfo;
	uint16_t e_res2[10];
	uint32_t e_lfanew;
} EXE_header_t;

typedef struct PE_signature
{
	uint16_t signature;
	uint16_t zero;
} PE_signature_t;

#define PE_SIGN 0x4550

typedef struct COFF_header
{
	uint16_t Machine;
	uint16_t NumberOfSections;
	uint32_t TimeDateStamp;
	uint32_t PointerToSymbolTable;
	uint32_t NumberOfSymbols;
	uint16_t SizeOfOptionalHeader;
	uint16_t Characteristics;
} COFF_header_t;

typedef struct PE_header_small
{
	/* Header Standard Fields */
	uint16_t Magic;
	uint8_t  MajorLinkerVersion;
	uint8_t  MinorLinkerVersion;
	uint32_t SizeOfCode;
	uint32_t SizeOfInitializedData;
	uint32_t SizeOfUninitializedData;
	uint32_t AddressOfEntryPoint;
	uint32_t BaseOfCode;
	uint32_t BaseOfData;
} PE_header_small_t;

#pragma pack(pop)

#define PE32 0x10b
#ifndef IMAGE_FILE_MACHINE_I386
#define IMAGE_FILE_MACHINE_I386 0x14c
#endif
#define SIZE_OF_PE32 224
#define EXE_SIGN 0x5a4D

static long EXE_offset(FILE *f, EXE_header_t *outEXE)
{
	long offset = 0;
	if(fread(outEXE, sizeof(EXE_header_t), 1, f) == 1)
	{
		if(outEXE->signature == EXE_SIGN)
		{
			if(outEXE->e_lfanew == 0)
			{
				offset = outEXE->blocks_in_file * 512;
				if(outEXE->bytes_in_last_block)
				{
					offset -= (512 - outEXE->bytes_in_last_block);
				}
			}
			else
			{
				offset = outEXE->e_lfanew;
			}
		}
	}
	
	return offset;
}

static DWORD GetEntryPoint(HMODULE hModule)
{
	DWORD result = -1;
	if(hModule)
	{
		char path[MAX_PATH];
		if(GetModuleFileNameA(hModule, path, sizeof(path)) != 0)
		{
			FILE *fr = fopen(path, "rb");
			if(fr)
			{
				EXE_header_t exe;
				long offset = EXE_offset(fr, &exe);
				if(offset > 0)
				{
					fseek(fr, offset, SEEK_SET);
					PE_signature_t pe_sign;
					fread(&pe_sign, sizeof(pe_sign), 1, fr);
					if(pe_sign.signature == PE_SIGN && pe_sign.zero == 0)
					{
						COFF_header_t coff;
						
						fread(&coff, sizeof(COFF_header_t), 1, fr);
						if(coff.Machine == IMAGE_FILE_MACHINE_I386)
						{
							PE_header_small_t pe;
							fread(&pe, sizeof(PE_header_small_t), 1, fr);
							if(pe.Magic == PE32)
							{
								result = pe.AddressOfEntryPoint;
							}
						}
					}
				}
				fclose(fr);
			}
		}
	}
	
	return result;
}

static DWORD GetBaseAddress(HMODULE hModule)
{
	DWORD result = -1;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
	MODULEENTRY32 modentry = { 0 };

	if(hSnapshot != NULL)
	{
		modentry.dwSize = sizeof(MODULEENTRY32);
		Module32First(hSnapshot, &modentry);
		do
		{
			if(modentry.hModule == hModule)
			{
				result = (DWORD)modentry.modBaseAddr;
				break;
			}
		} while(Module32Next(hSnapshot, &modentry));

		CloseHandle(hSnapshot);
	}

	return result;
}

BOOL patch_install(const char *libname, const char *procname, void *trampoline)
{
	HMODULE dll = GetModuleHandleA(libname);
	BOOL rc = FALSE;
	
	if(dll != NULL)
	{
		void *addr = NULL;
		
		if(procname != NULL)
		{
			addr = GetProcAddress(dll, procname);
		}
		else
		{
			DWORD base = GetBaseAddress(dll);
			DWORD entry = GetEntryPoint(dll);
			
			if(base != -1 && entry != -1)
			{
				addr = (void*)(base + entry);
			}
		}
		
		if(addr)
		{
			rc = patch_install_address(addr, trampoline);
		}
		else
		{
			TRACE("PATCH", "addr is NULL!");
		}
	}
	
	return rc;
}

BOOL patch_uninstall(const char *libname, const char *procname, void *trampoline)
{
	HMODULE dll = GetModuleHandleA(libname);
	BOOL rc = FALSE;
	
	if(dll != NULL)
	{
		void *addr = NULL;
		
		if(procname != NULL)
		{
			addr = GetProcAddress(dll, procname);
		}
		else
		{
			DWORD base = GetBaseAddress(dll);
			DWORD entry = GetEntryPoint(dll);
			
			if(base != -1 && entry != -1)
			{
				addr = (void*)(base + entry);
			}
		}
		
		if(addr)
		{
			rc = patch_uninstall_address(addr, trampoline);
		}
		else
		{
			TRACE("PATCH", "addr is NULL!");
		}
	}
	
	return rc;
}

int patch_validate(const char *libname, const char *procname, void *trampoline)
{
	HMODULE dll = GetModuleHandleA(libname);
	int rc = PATCH_NOTAPPLICABLE;
	
	if(dll != NULL)
	{
		void *addr = NULL;
		
		if(procname != NULL)
		{
			addr = GetProcAddress(dll, procname);
		}
		else
		{
			DWORD base = GetBaseAddress(dll);
			DWORD entry = GetEntryPoint(dll);
			
			if(base != -1 && entry != -1)
			{
				addr = (void*)(base + entry);
			}
		}
		
		if(addr)
		{
			rc = patch_validate_address(addr, trampoline);
		}
	}

	return rc;
}
