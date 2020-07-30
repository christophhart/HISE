/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "basic/function_overload"
END_TEST_DATA
*/

int get(double input){ return 3;}
int get(float input){ return 9;}
			
int main(int input)
{
	float v = (float)input;
    return get(v);
}

