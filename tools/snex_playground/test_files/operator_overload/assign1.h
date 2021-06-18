/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 90
  error: ""
  filename: "operator_overload/assign1"
END_TEST_DATA
*/

struct X
{
	auto& operator=(float nv)
	{
		v = (int)nv;
		
		return *this;
	}
	
	int v = 0;
};

int main(int input)
{
	X i;
	
	i = 90.0f;
	
	return i.v;
}

