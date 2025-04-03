#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "../vmsetup.h"

int main(int argc, char **argv)
{
	const char *s = NULL;
	if(argc >= 3)
	{
		s = vmhal_setup_str(argv[1], argv[2], TRUE);
		printf("[%s %s] = \"%s\"\n", argv[1], argv[2], s);
	}
	else if(argc == 2)
	{
		s = vmhal_setup_str(NULL, argv[1], TRUE);
		printf("[%s] = \"%s\"\n", argv[1], s);
	}
	else
	{
		printf("Usage: %s [category] name\n", argv[0]);
	}
	
	return EXIT_SUCCESS;
}
