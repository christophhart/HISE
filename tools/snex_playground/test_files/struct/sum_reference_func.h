/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 56
  error: ""
  filename: "struct/sum_reference_func"
END_TEST_DATA
*/

struct X
{
    int x1 = 2;
    int x2 = 54;
};

span<float, 4> data = { 2.0f };

int sumX(X& x)
{
    return x.x1 + x.x2;
}


int main(int input)
{
	X x;
	
  return sumX(x);
}



