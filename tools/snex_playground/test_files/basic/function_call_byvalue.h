/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "basic/function_call_byvalue"
END_TEST_DATA
*/



struct X
{
    int v = 12;
};

using MySpan = span<X, 2>;

void resetObject(MySpan copy)
{
    copy[0].v = 0;
}

MySpan x;

int main(int input)
{
	resetObject(x);
	
	return x[0].v;
}

