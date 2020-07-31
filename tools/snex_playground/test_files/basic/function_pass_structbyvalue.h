/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 200
  error: ""
  filename: "basic/function_pass_structbyvalue"
END_TEST_DATA
*/

struct X
{
    int v = 110;
    float d = 1.0f;
    float x = 4.0f;
    float y = 8.0f;
    span<int, 40> data = { 90 };
};

int getValueFromX(X copy)
{
    copy.v = 9;
    return copy.data[0];
}

X x;

int main(int input)
{
	return x.v + getValueFromX(x); 
}

