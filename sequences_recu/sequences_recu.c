#include <stdio.h>
#include <time.h>

void recu_sequence(char * alph, char * password, int pos)
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

int main(int argc, char * argv[])
{
  int i;
  int length = 30;
  char * alph = "ab";
  char password[length + 1];

  password[length] = 0;

  for (i = 0; i < length; i++)
    {
      password[i] = alph[0];
    }

  clock_t start = clock();
  
  recu_sequence(alph, password, length - 1);

  clock_t finish = clock();

  printf("Elapsed: %f seconds\n", (double)(finish - start) / CLOCKS_PER_SEC);

  char * alph_second = "abcdefghijklmnopqrstuvwxyz[],.+-";
  int length_second = 6;
  char password_second[length_second + 1];

  password_second[length_second] = 0;

  for (i = 0; i < length_second; i++)
    {
      password_second[i] = alph_second[0];
    }
  
  start = clock();
  
  recu_sequence(alph_second, password_second, length_second - 1);

  finish = clock();

  printf("Elapsed: %f seconds\n", (double)(finish - start) / CLOCKS_PER_SEC);
  
  return 0;
}
