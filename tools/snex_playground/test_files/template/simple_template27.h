/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 12
  output: 4.5
  error: ""
  filename: "template/simple_template27"
END_TEST_DATA
*/

template <int NumElements> struct X
{
    span<float, NumElements> data = { 4.0f };
};

float main(float input)
{
  X<4> obj2;
  X<5> obj1 = { { 0.5f }};
  
	
	
	
	return obj1.data[0] + obj2.data[2];
}

