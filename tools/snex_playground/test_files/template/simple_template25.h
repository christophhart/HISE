/*
BEGIN_TEST_DATA
  f: main
  ret: double
  args: double
  input: 12
  output: 4
  error: ""
  filename: "template/simple_template25"
END_TEST_DATA
*/

template <typename T> struct Dual
{
    T sum()
    {
        return data[0] + data[1];
    }
    
    span<T, 2> data;
};




double main(double input)
{
	Dual<double> d = { {1.0, 3.0} };
	
	return d.sum();
}

