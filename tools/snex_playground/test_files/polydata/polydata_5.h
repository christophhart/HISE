/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 1
  voiceindex: 3
  error: ""
  filename: "polydata/polydata_5"
END_TEST_DATA
*/

PolyData<int, 15> data;


void prepare(PrepareSpecs ps)
{
  data.prepare(ps);
}

int main(int input)
{
	int counter = 0;
	

	for(auto& s: data)
		counter++;

	return counter;
}


  
