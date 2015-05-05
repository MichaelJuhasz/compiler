int factorial(int n);
int fib(int n);
void print_number(int n);
void print_string(char *s);

void main(void) {
  int f; 
  int facs[10];
  int fibs[10];

  for(f = 3; f < 10; f++)
  {
    facs[f] = factorial(f);
    fibs[f] = fib(f);
  }

  for(f = 3; f < 10; f++)
  {
     print_string("Fibonacci number ");
     print_number(f);
     print_string(": ");
     print_number(fibs[f]);
     print_string("\n");

     print_string("Factorial of ");
     print_number(f);
     print_string(": ");
     print_number(facs[f]);
     print_string("\n");

     print_string("Factorial of Fibonacci number ");
     print_number(f);
     print_string(": ");
     print_number(factorial(fib(f)));
     print_string("\n");
     print_string("\n");
  }
}

int factorial(int x) {
  if(x <= 1)
    return 1;
  else
    return x*factorial(x-1);
}

int fib(int n) {
  return (n < 2 ? n : fib(n - 1) + fib(n - 2));
}