/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "index/index14"
END_TEST_DATA
*/

struct MyClass
{
	span<int, 4> data = { 1, 2, 12, 4 };

	int process() const
	{
		index::clamped<0, false> idx;
		idx = 2;
		
		return data[idx];
	}
	
};

int main(int input)
{
	MyClass obj;
	
	return obj.process();
}

