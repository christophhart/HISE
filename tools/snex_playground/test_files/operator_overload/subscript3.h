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
		
		index::unsafe<0> idx(i);
		
		copy[0] = left[idx];
		copy[1] = right[idx];
		
		return copy;
	}
	
private:
	
	span<float, 12> left = {14.0f};
	span<float, 12> right = {18.0f};
};

StereoBuffer obj;

int main(int input)
{
	int i = 3;
	
	auto frame = obj[i];
	
	return (int)frame[0];
	
}

