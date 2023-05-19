/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 17
  error: ""
  filename: "default_arg/default_4"
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
	
	return data1.size() + data2.size() + data3.size();
}

