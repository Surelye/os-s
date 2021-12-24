#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <crypt.h>
#include <stdbool.h>

typedef enum {
  ITER,
  RECU
  } launch_mode_t;

typedef struct {
  char *  alph;
  int length;
  char * hash;
  launch_mode_t launch_mode;  
} config_t;

bool check_password(char * password, char * hash)
{  
  char * password_hash = crypt(password, hash);
  return (strcmp(password_hash, hash) == 0);
}

bool recu_sequence(char * password, int pos, config_t * config)
{
  int i;

  if (pos < 0)
    {
      return (check_password(password, config->hash));
    }
  else for (i = 0; config->alph[i]; i++)
    {
      password[pos] = config->alph[i];
      if (recu_sequence(password, pos - 1, config))
	return true;
    }

  return false;
}

bool iter_sequence(char * password, config_t * config)
{  
  int index[config->length];
  int i, alph_length = strlen(config->alph) - 1;

  for (i = 0; i < config->length; i++)
    {
      index[i] = 0;
      password[i] = config->alph[0];
    }
  
  while (1)
    {
      // printf("%s\n", password);
      
      if (check_password(password, config->hash))
	return true;
      
      for (i = config->length - 1; (i >= 0) && (index[i] == alph_length); i--)
	{
	  index[i] = 0;
	  password[i] = config->alph[0];
	}

      if (i < 0)
	break;

      password[i] = config->alph[++index[i]];
    }

  return false;
}

void parse_params(config_t * config, int argc, char * argv[])
{
  int placeholder = 0;

  while ((placeholder = getopt(argc, argv, "a:l:m:h:")) != -1)
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
	  break;

	case 'h':
	  config->hash = optarg;
	  break;
	}
    }
}

int main(int argc, char * argv[])
{
  config_t config = {
    .alph = "0",
    .length = 3,
    .hash = "NO_INPUT_HASH",
    .launch_mode = RECU
  };
  
  parse_params(&config, argc, argv);
  
  char password[config.length + 1];
  password[config.length] = 0;
  
  int i;
  bool found = false;
  
  for (i = 0; i < config.length; i++)
    {
      password[i] = config.alph[0];
    }
  
  if (config.length > 0 && strlen(config.alph) > 0)
    {
      switch(config.launch_mode)
	{
	case ITER:
	  found = iter_sequence(password, &config);
	  printf("Finished with iterative algorithm.\n");
	  break;
	  
	case RECU:
	  found = recu_sequence(password, config.length - 1, &config);
  	  printf("Finished with recursive algorithm.\n");
	  break;
	}
    }

  // printf("%s\n", crypt("aaa", "bb"));
  
  if (found)
    printf("Password is: %s.\n", password);
  else
    printf("Password is not found.\n");
    
  
  return 0;
}
