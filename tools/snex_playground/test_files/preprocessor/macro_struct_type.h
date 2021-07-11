/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 3
  error: ""
  filename: "preprocessor/macro_struct_type"
END_TEST_DATA
*/

#define VALUE_CLASS(type) struct Object { type value = 3; }



VALUE_CLASS(int);


int main(int input)
{
	Object x;
	
	return x.value;
}

