/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.0f
  output: 2.0f
  error: ""
  filename: "span/local_span_anonymous_scope"
END_TEST_DATA
*/

struct Object
{
    int z = 5;
    double d = 1.0;
};

float main(float input)
{
	Object d1, d2;
	
	return (float)d1.d + (float)d2.d;
}

