/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 3
  error: ""
  filename: ""
END_TEST_DATA
*/

struct X
{
	template <int C> void processFrame(span<float, C>& data)
	{
		for(auto& s: data)
			sum += s;
	}
	
	float sum = 0.0f;
};

X obj;
span<float, 2> data = { 1.0f, 2.0f };

int test(int input)
{
	obj.processFrame(data);
	return (int)obj.sum;
}

