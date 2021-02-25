/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 90
  error: ""
  filename: "polydata/polydata_2"
END_TEST_DATA
*/

PolyData<int, 1> data;

int main(int input)
{
	for(auto& e: data)
	    e = 90;
	

	//Console.print(data);
	    
	return data.get();
}

