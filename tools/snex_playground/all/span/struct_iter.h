/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 6
  error: ""
  filename: "span/struct_iter"
END_TEST_DATA
*/


struct X 
{ 
	double unused = 2.0; 
	int value = 2; 
};

span<X, 3> data;

int main(int input)
{
	for(auto& s: data)
	    input += s.value;
	    
	return input;	
}

