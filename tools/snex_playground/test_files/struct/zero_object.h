/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 19
  error: ""
  filename: "struct/zero_object"
END_TEST_DATA
*/

int i = 0;

struct X
{
	int get()
	{
		return i;
	}
	
	void inc()
	{
		i++;
	}
};

void inc(X& o)
{
	o.inc();
}

int main(int input)
{
	X obj;
	inc(obj);	

	span<X, 18> data;

	for(auto& a: data)
	    inc(a);

	
	return i;
}

