#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
    BM_ITER,
    MR_RECU
  } launch_mode_t;

typedef struct {
  char *  alph;
  int length;
  launch_mode_t launch_mode;
} config_t;

void recu_sequence(char * alph, char * password, int pos) // to get all sequences put pos equal to length - 1
{
  int i;

  if (pos < 0)
    {
      printf("%s\n", password);
    }
  else for (i = 0; alph[i]; i++)
    {
      password[pos] = alph[i];
      recu_sequence(alph, password, pos - 1);
    }
}

void iter_sequence(char * alph, int length)
{
  int current_symbol[length];
  char current_password[length];
  int current_position = length - 1; // the index of symbol we currently modify
  int i;
  int alphabet_length = strlen(alph);

  for (i = 0; i < length; i++)
    {
      current_password[i] = alph[0];
      current_symbol[i] = 0;
    }

  while (1)
    {
      printf("%s\n", current_password);

      if (current_symbol[current_position] < alphabet_length - 1)
	{
	  current_password[current_position] = alph[++current_symbol[current_position]];
	}
      else
	{
	  while (current_symbol[current_position] == alphabet_length - 1)
	    {
	      current_symbol[current_position] = 0;
	      current_password[current_position--] = alph[0];
	    }

	  if (current_position < 0)
	    {
	      break;
	    }
	  
	  current_password[current_position] = alph[++current_symbol[current_position]];
	  current_position = length - 1;
	}
    }
  
}

void parse_params(config_t * config, char * argv[])
{
  config->alph = argv[1];
  config->length = atoi(argv[2]);

  if (strcmp("BM_ITER", argv[3]) == 0 || atoi(argv[3]) == 0)
    {
      config->launch_mode = BM_ITER;
    }
  else if (strcmp("MR_RECU", argv[3]) == 0 || atoi(argv[3]) == 1)
    {
      config->launch_mode = MR_RECU;
    }
}



int main(int argc, char * argv[])
{
  config_t config = {"0", 3, MR_RECU};

  parse_params(&config, argv);

  char password[config.length + 1];
  password[config.length] = 0;
  
  int i;
  
  for (i = 0; i < config.length; i++)
    {
      password[i] = config.alph[0];
    }
  
  if (config.length > 0 && strlen(config.alph) > 0)
    {
      switch(config.launch_mode)
	{
	case 0:
	  iter_sequence(config.alph, config.length);
	  printf("Finished with iterative algorithm.\n");
	  break;

	case 1:
	  recu_sequence(config.alph, password, config.length - 1);
  	  printf("Finished with recursive algorithm.\n");
	  break;

	default:
	  printf("Oops ! Something went wrong.\n");
	}
    }
  
  return 0;
}