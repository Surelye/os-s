#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

typedef enum {
  ITER,
  RECU
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

/*
void iter_sequence(char * alph, int length)
{
  char current_password[length];
  int current_symbol[length];
  int current_position = length - 1; // the index of symbol we currently modify
  int i;
  int alphabet_length = 0;

  for (i = 0; alph[i]; i++)
    {
      alphabet_length++; 
    }  

  for (i = 0; i < length; i++)
    {
      current_password[i] = alph[0];
      current_symbol[i] = 0;
    }

  while (1)
    {
      // printf("%s\n", current_password);
      
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
*/

void iter_sequence(char * alphabet, int length)
{
  int password[length];
  int current_position = length - 1; // the index of symbol we currently modify
  int i;
  int alphabet_length = strlen(alphabet);
  int flag = 0;
  
  for (i = 0; i < length; i++)
    {
      password[i] = 0;
    }
  
  while (1)
    {
      //for (i = 0; i < length; i++)
      //{
      //printf("%c", alphabet[password[i]]);
      //	}
      //printf("\n");
      
      while (current_position >= 0 && password[current_position] == alphabet_length - 1 && ++flag)
	{
	  password[current_position] = 0;
	  current_position--;
	}
      
      if (current_position < 0)
	{
	  break;
	}
      
      password[current_position]++;
      
      if (flag)
	{
	  flag = 0;
	  current_position = length - 1;
	}
    }
}

void parse_params(config_t * config, int argc, char * argv[])
{
  int placeholder = 0;

  while ((placeholder = getopt(argc, argv, "a:l:m:")) != -1)
    {
      switch (placeholder)
	{
	case 'a':
	  config->alph = optarg;
	  break;

	case 'l':
	  config->length = atoi(optarg);
	  break;

	case 'm':
	  if (strcmp("ITER", optarg) == 0)
	    {
	      config->launch_mode = ITER;
	    }
	  else if (strcmp("RECU", optarg) == 0)
	    {
	      config->launch_mode = RECU;
	    }
	}
    }
}

int main(int argc, char * argv[])
{
  config_t config = {"0", 3, RECU};

  parse_params(&config, argc, argv);
  
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
	case ITER:
	  iter_sequence(config.alph, config.length);
	  printf("Finished with iterative algorithm.\n");
	  break;

	case RECU:
	  recu_sequence(config.alph, password, config.length - 1);
  	  printf("Finished with recursive algorithm.\n");
	  break;
	}
    }
  
  return 0;
}
