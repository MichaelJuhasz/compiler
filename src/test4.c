int absolute(int x) 
{
	if(x < 0)
		return -x;
	else 
		return x;
}

void main(int i, int j, int *p)
{
	i = 5;
	p = &i;
	j = *p;
	i = (int)(char)j;

	i = j + 10 * 9 / i;
	
	goto errorDetected;
	i++;
	errorDetected: i--;
	
	if(i > 0)
	{
		i += j++;
	}
	else 
	{
		j += i++;
	}

	while(i < j)
	{
		j &= 8;
	}

	do {
		if(i == 0)
			break;
		i--;
	}
	while (i > 0);

	int f;
	int g;
	for(f = 0; f < i; f++)
	{
		for(g = 0; g < f; g++)
		{
			g = f;
		}
	}

	i = absolute(i);

	int arr[10];
	arr[0] = 1;
	arr[1] += arr[0];

	*(p++) += 8;
}