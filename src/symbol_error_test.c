void main(char i);
void main(int i){int i; i = g;}
void main(int i);

void func(int i, int j);
void func(int i){}

int blarg(void){}
int blarg(void);

void wrong(void);
int wrong(void){}

int errors(void) {
	int ar[][];
	int ar[9][];
	int ar[9][][10];
}

int arr[](void){}

void (*foo)(int g);