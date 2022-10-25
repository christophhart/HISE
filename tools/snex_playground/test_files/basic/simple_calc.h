/*
BEGIN_TEST_DATA
  f: main
  ret: T
  args: T
  input: 4
  output: -1136
  error: ""
  filename: "basic/simple_calc"
END_TEST_DATA
*/

T calc(T input)
{
	T x = input > (T)9 ? (T)5 : (T)2;
	T y = input * input - (T)4;
	T z = (x + y) * (T)4;
	
	
	T a = x - input + (T)6;
	T b = a > x ? (T)2 : (T)5;
	T c = z * a + (T)5;
	
	T s2 = (T)12 + a * (b - c);
	T s1 = x + y - (z + (T)2 * input);
	
	return s1 + s2;
}

T main(T input)
{
	return calc(input) + calc(input * (T)2);
}



