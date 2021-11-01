#include <stdio.h>
#include <unistd.h>

int main(int argc, char * argv[])
{
  int placeholder = 0;

  while ((placeholder = getopt(argc, argv, "ab:C::d")) != -1)
    {
      switch (placeholder)
	{
	case 'a':
	  printf("Found argument \"a\".\n");
	  break;
	  
	case 'b':
	  printf("Found argument \"b = %s\".\n", optarg);
	  break;

	case 'C':
	  printf("Found argument \"C = %s\".\n", optarg);
	  break;

	case 'd':
	  printf("Found argument \"d\"\n");
	  break;

	case '?':
	  printf("Error found !\n");
	  break;
	}
    }
}