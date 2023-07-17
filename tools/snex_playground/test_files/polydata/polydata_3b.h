/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 90
  error: ""
  filename: "polydata/polydata_3b"
END_TEST_DATA
*/

PolyData<double, 180> data;

span<double, 8> d = {0.0};

int main(int input)
{
  for(auto s: data)
	    s = 90.0;

	return (int)data.get();
	    
}

