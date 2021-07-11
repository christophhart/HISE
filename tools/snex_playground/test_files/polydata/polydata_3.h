/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 90
  error: ""
  filename: "polydata/polydata_3"
END_TEST_DATA
*/

PolyData<int, 18> data;

int main(int input)
{
	for(auto& s: data)
	    s = 90;

	return data.get();
	    
}

