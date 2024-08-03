/******************************************************************************
 * Copyright (c) 2024 Jaroslav Hensl                                          *
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
#include <ddraw.h>
#include <ddrawi.h>
#include <stddef.h>
#include <tlhelp32.h>

#include "vmdahal32.h"

#include "vmhal9x.h"

#include "nocrt.h"

#ifdef DEBUG

static MODULEENTRY32 modentry;

void PrintModules(FILE *f)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);

	if(hSnapshot != NULL)
	{
		fprintf(f, "=== modules ===\r\n");
		
		memset(&modentry, 0, sizeof(MODULEENTRY32));

		modentry.dwSize = sizeof(MODULEENTRY32);
		Module32First(hSnapshot, &modentry);
		do
		{
			fprintf(f, "%s: base=0x%08X, size=0x%08X\r\n",
				modentry.szModule, (DWORD)modentry.modBaseAddr, modentry.modBaseSize
			);
		} while(Module32Next(hSnapshot, &modentry));

		CloseHandle(hSnapshot);
	}
}

void PrintStack(FILE *f, DWORD *esp)
{
	int i = 0;
	fprintf(f, "=== stack ===\r\n");
	if(esp != NULL)
	{
		for(i = 0; i < 32; i++)
		{
			fprintf(f, "0x%08X: %08X\r\n", &esp[i], esp[i]);			
		}
	}
}

void PrintRegistry(FILE *f, PCONTEXT ctx)
{
	fprintf(f, "=== cpu ===\r\n");
	fprintf(f, "EAX: 0x%08X\r\n", ctx->Eax);
	fprintf(f, "EBX: 0x%08X\r\n", ctx->Ebx);
	fprintf(f, "ECX: 0x%08X\r\n", ctx->Ecx);
	fprintf(f, "EDX: 0x%08X\r\n", ctx->Edx);
	fprintf(f, "ESI: 0x%08X\r\n", ctx->Esi);
	fprintf(f, "EDI: 0x%08X\r\n", ctx->Edi);
	fprintf(f, "EBP: 0x%08X\r\n", ctx->Ebp);
	fprintf(f, "ESP: 0x%08X\r\n", ctx->Esp);
}

void PrintCode(FILE *f, DWORD addr)
{
	int i = 0;
	fprintf(f, "=== code ===\r\n    ");
	if(addr > 1024*1024) // not tries read something lower than first 1MB
	{
		BYTE *ptr = (BYTE *)addr;
		for(i = -32; i < 0; i++)
		{
			fprintf(f, "%02X ", ptr[i]);
		}
		fprintf(f, "\r\n--> ");
		for(i = 0; i <= 32; i++)
		{
			fprintf(f, "%02X ", ptr[i]);
		}
		fprintf(f, "\r\n");
	}
	fprintf(f, "=== end of dump ===\r\n");
}

LONG WINAPI VmhalExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	FILE *f;

	f = fopen("C:\\vmhal.dump", "ab");
	if(f)
	{
		fprintf(f, "Exception %X at 0x%08X\r\n",
			ExceptionInfo->ExceptionRecord->ExceptionCode,
			(DWORD)ExceptionInfo->ExceptionRecord->ExceptionAddress
		);
		
		PrintModules(f);
		PrintRegistry(f, ExceptionInfo->ContextRecord);
		PrintStack(f, (DWORD*)ExceptionInfo->ContextRecord->Esp);
		PrintCode(f, (DWORD)ExceptionInfo->ExceptionRecord->ExceptionAddress);
		
		fclose(f);
	}
	
	return EXCEPTION_EXECUTE_HANDLER;
}

void SetExceptionHandler()
{
	SetUnhandledExceptionFilter(VmhalExceptionFilter);
}

#endif /* DEBUG */
