/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 19
  error: ""
  filename: "basic/set_with_branch"
END_TEST_DATA
*/

struct Obj
{
	float value = 2.0f;
	float value2 = 1.0f;
	
	void set(int input, float v)
	{
		if(input == 0)
			value = v;
		else
			value = v * 2.0f;
	}
};

Obj x;

int main(int input)
{
	x.set(input, 19.0f);
	return x.value;
}

