/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "struct/basic_copy"
END_TEST_DATA
*/

struct X
{
    
    float d = 12.0f;
    int x = 9;
    double f = 16.0;
};

X x;


void change(X obj)
{
    obj.x = 5;
}

int main(int input)
{
	change(x);
	
	return x.x;
}

