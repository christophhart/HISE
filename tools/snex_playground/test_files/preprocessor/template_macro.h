/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: ""
  filename: "preprocessor/template_macro"
END_TEST_DATA
*/

#define func(x) template <int C> void x(int& value) { value = C; }

func(setToValue);


int main(int input)
{
	setToValue<5>(input);
	
	return input;
}

