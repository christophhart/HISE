/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 1
  error: ""
  filename: "polydata/polydata_2"
END_TEST_DATA
*/

PolyData<int, 18> data;

int main(int input)
{
	int counter = 0;
	

	for(auto& s: data)
		counter++;

	return counter;
}

