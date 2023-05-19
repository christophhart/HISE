/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 22
  error: ""
  filename: "default_arg/dyn_size_defaultarg"
END_TEST_DATA
*/


span<int, 10> data = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

dyn<int> data1;
dyn<int> data2;
dyn<int> data3;



int main(int input)
{
	data1.referTo(data, data.size());
	data2.referTo(data, 3);
	data3.referTo(data, 4, 2);
	
	auto sizeSum = data1.size() + data2.size() + data3.size();
	auto firstSum = data1[0] + data2[0] + data3[0];
	
	return sizeSum + firstSum;
}

