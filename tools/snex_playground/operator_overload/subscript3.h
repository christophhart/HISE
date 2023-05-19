/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 14
  error: ""
  filename: "operator_overload/subscript3"
END_TEST_DATA
*/

struct StereoBuffer
{
	span<float, 2> operator[](int i)
	{
		span<float, 2> copy;
		
		copy[0] = left[i];
		copy[1] = right[i];
		
		return copy;
	}
	
private:
	
	span<float, 12> left = {14.0f};
	span<float, 12> right = {18.0f};
};

StereoBuffer obj;

int main(int input)
{
	int i = 1;
	
	auto frame = obj[i];
	
	return (int)frame[0];
	
}

