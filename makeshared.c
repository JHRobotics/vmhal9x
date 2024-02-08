/******************************************************************************
 * Copyright (c) 2023 Jaroslav Hensl                                          *
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/*
 * PE Documentation here:
 *  https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
 *
 * PE checksum algorithm:
 *  https://bytepointer.com/resources/microsoft_pe_checksum_algo_distilled.htm
 *
 * Discussion about PE checksum (list of examples how don't do it):
 *  https://stackoverflow.com/questions/6429779/can-anyone-define-the-windows-pe-checksum-algorithm
 *
 */

#define PE_FILE_MAGIC {'P','E',0,0}

#define PE32 0x10b
#define IMAGE_FILE_MACHINE_I386 0x14c
#define SIZE_OF_PE32 224

#define IMAGE_SCN_MEM_SHARED 0x10000000
#define IMAGE_SCN_MEM_DISCARDABLE 0x02000000

#pragma pack(push)
#pragma pack(1)

typedef struct _hcoff
{
	uint16_t Machine;
	uint16_t NumberOfSections;
	uint32_t TimeDateStamp;
	uint32_t PointerToSymbolTable;
	uint32_t NumberOfSymbols;
	uint16_t SizeOfOptionalHeader;
	uint16_t Characteristics;
} hcoff_t;

typedef struct _hpe
{
	// Header Standard Fields
	uint16_t Magic;
	uint8_t  MajorLinkerVersion;
	uint8_t  MinorLinkerVersion;
	uint32_t SizeOfCode;
	uint32_t SizeOfInitializedData;
	uint32_t SizeOfUninitializedData;
	uint32_t AddressOfEntryPoint;
	uint32_t BaseOfCode;
	uint32_t BaseOfData;
	// Header Windows-Specific Fields
	uint32_t ImageBase;
	uint32_t SectionAlignment;
	uint32_t FileAlignment;
	uint16_t MajorOperatingSystemVersion;
	uint16_t MinorOperatingSystemVersion;
	uint16_t MajorImageVersion;
	uint16_t MinorImageVersion;
	uint16_t MajorSubsystemVersion;
	uint16_t MinorSubsystemVersion;
	uint32_t Win32VersionValue;
	uint32_t SizeOfImage;
	uint32_t SizeOfHeaders;
	uint32_t CheckSum;
	uint16_t Subsystem;
	uint16_t DllCharacteristics;
	uint32_t SizeOfStackReserve;
	uint32_t SizeOfStackCommit;
	uint32_t SizeOfHeapReserve;
	uint32_t SizeOfHeapCommit;
	uint32_t LoaderFlags;
	uint32_t NumberOfRvaAndSizes;
} hpe_t;

typedef struct _hsection
{
	uint8_t  Name[8];
	uint32_t VirtualSize;
	uint32_t VirtualAddress;
	uint32_t SizeOfRawData;
	uint32_t PointerToRawData;
	uint32_t PointerToRelocations;
	uint32_t PointerToLinenumbers;
	uint16_t NumberOfRelocations;
	uint16_t NumberOfLinenumbers;
	uint32_t Characteristics;
} hsection_t;

#pragma pack(push)

int main(int argc, char **argv)
{
	const uint8_t pe_file_header[4] = PE_FILE_MAGIC;
	uint8_t pe_test[4];
	hcoff_t coff;
	hpe_t   pe;
	hsection_t section;
	int i, c;
	char section_name[9] = {0};
	int status = EXIT_FAILURE;
	long checksum_pos = 0;
	size_t fpos;
	uint32_t checksum;
	
	if(argc < 2)
	{
		printf("Usage:\n\t%s file.dll\n", argv[0]);
		status = EXIT_SUCCESS;
		goto exit_only;
	}
	
	FILE *fr = fopen(argv[1], "r+b");
	if(!fr)
	{
		printf("failed to open %s for R/W\n", argv[1]);
		goto exit_only;
	}
	
	fread(pe_test, sizeof(pe_test), 1, fr);
	
	while(!feof(fr))
	{
		if(memcmp(pe_test, pe_file_header, sizeof(pe_test)) == 0)
		{
			break;
		}
		c = fgetc(fr);
		pe_test[0] = pe_test[1];
		pe_test[1] = pe_test[2];
		pe_test[2] = pe_test[3];
		pe_test[3] = c;
	}
	
	if(feof(fr))
	{
		printf("PE magic not found!\n");
		goto exit_close;
	}
	
	if(fread(&coff, sizeof(hcoff_t), 1, fr) != 1)
	{
		printf("Unexpected EOF in COFF\n");
		goto exit_close;
	}
	
	if(coff.Machine != IMAGE_FILE_MACHINE_I386)
	{
		printf("Machine %04X, expected i386 (%04X)", coff.Machine, IMAGE_FILE_MACHINE_I386);
		goto exit_close;
	}
	
	if(fread(&pe, sizeof(hpe_t), 1, fr) != 1)
	{
		printf("Unexpected EOF in PE\n");
		goto exit_close;
	}
	
	if(pe.Magic != PE32)
	{
		printf("Expected PE32, found: %04X\n", pe.Magic);
		goto exit_close;
	}
	
	if(pe.ImageBase < 0x80000000UL)
	{
		printf("Image base must be in above 2G space, current value: %08X\n", pe.ImageBase);
		goto exit_close;
	}
	
	printf("Image base: %08X\n", pe.ImageBase);
	checksum_pos = ftell(fr) + offsetof(hpe_t, CheckSum) - sizeof(hpe_t);
	
	fseek(fr, SIZE_OF_PE32-sizeof(hpe_t), SEEK_CUR);
	
	//printf("Position: %ld\n", ftell(fr));

	for(i = 0; i < coff.NumberOfSections; i++)
	{
		if(fread(&section, sizeof(hsection_t), 1, fr) != 1)
		{
			printf("Unexpected EOF in section\n");
			goto exit_close;
		}
		
		memcpy(section_name, section.Name, 8);
		printf("Section: %8s, flags = %08X", section_name, section.Characteristics);
		
		if((section.Characteristics & IMAGE_SCN_MEM_SHARED) == 0)
		{
			if((section.Characteristics & IMAGE_SCN_MEM_DISCARDABLE) == 0)
			{
				section.Characteristics |= IMAGE_SCN_MEM_SHARED;
				fseek(fr, -sizeof(hsection_t), SEEK_CUR);
				fwrite(&section, sizeof(hsection_t), 1, fr);
				fflush(fr);
				
				printf(" -> %08X", section.Characteristics);
			}
		}
		printf("\n");
	}
	
	/* PE checksum recalculation */
	fpos = 0;
	checksum = 0; /* initial value */
	fseek(fr, 0, SEEK_SET);
	while(!feof(fr))
	{
		uint16_t word = 0;
		
		/* first byte */
		if(fpos == checksum_pos) /* ignore original checksum */
		{
			fseek(fr, 4, SEEK_CUR);
			fpos += 4;
		}
		c = fgetc(fr);
		if(c == EOF)
		{
			break;
		}
		else
		{
			fpos++;
		}
		word = c;
		
		/* second byte */
		if(fpos == checksum_pos) /* ignore original checksum */
		{
			fseek(fr, 4, SEEK_CUR);
			fpos += 4;
		}
		c = fgetc(fr);
		if(c == EOF)
		{
			c = 0; /* when is file not word aligned, assume extra byte is 0 */
		}
		else
		{
			fpos++;
		}
		word |= c << 8; /* low endian */
		
		/* update checksum */
		checksum += word;
		checksum = (checksum >> 16) + (checksum & 0xffff);
	}
	
	checksum = (checksum >> 16) + (checksum & 0xffff);
	checksum += fpos; /* finaly sum file size with checksum */
	
	printf("Checksum: %08X", pe.CheckSum);
	if(pe.CheckSum != checksum)
	{
		fseek(fr, checksum_pos, SEEK_SET);
		fwrite(&checksum, 4, 1, fr);
		
		printf(" -> %08X", checksum);
	}
	printf("\n");
	
	status = EXIT_SUCCESS;

exit_close:
	fclose(fr);
exit_only:
	return status;
}

