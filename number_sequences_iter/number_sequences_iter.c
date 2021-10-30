#include <stdio.h>

void number_printing(int * sequence, int length)
{
  int i;
  
  for (i = 0; i < length; i++)
    {
      printf("%d", sequence[i]);
    }
  
  printf("\n");
}

void iter_numbers(int length, int max_digit)
{
  int i;
  int current_digit = length - 1;
  int current_password[length];
  
  for (i = 0; i < length; i++)
    {
      current_password[i] = 1;
    }

  while (1)
    {
      number_printing(current_password, length);
      
      if (current_password[current_digit] < max_digit)
	{
	  current_password[current_digit]++;
	}
      else
	{
	  while (current_password[current_digit] == max_digit)
	    {
	      current_password[current_digit] = 1;
	      current_digit--;
	    }

	  if (current_digit < 0)
	    {
	      break;
	    }
	  else
	    {
	      current_password[current_digit]++;
	      current_digit = length - 1;
	    }
	}
    }
}

int main(int argc, char * argv[])
{
  int length, max_digit;
  
  // Input the length of the sequence
  scanf("%d", &length);

  // Input the maximal digit of the sequence, not greater than 9
  scanf("%d", &max_digit);
  
  iter_numbers(length, max_digit); // length, then max_digit
}

