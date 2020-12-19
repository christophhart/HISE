/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 11
  output: 18
  error: ""
  filename: "00 Examples/07_templates"
END_TEST_DATA
*/

/** Introduction into (C++) templates and some
    of the most important use cases in SNEX
*/


/** This class template will contain a span with
    a length specified by the Size template parameter
*/
template <int Size> class IntArray
{
private:

	// Since the Size variable is a compile time
	// constant we can use it to declare the inner type.
	span<int, Size> data = {0};
	
public:

	/** Functions can also be templated. In this case
	    we can use the Index variable directly because
	    it is a compile time constant.
	*/
	template <int Index> void set(int value)
	{
		data[Index] = value;
	}
	
	/** For comparison, the get method is not templated,
	    so you need to convert the subscript variable
	    to a IndexType. 
	*/
	int get(int index) const
	{
		auto i = IndexType::wrapped(data);
		i = index;
		return data[i];
	}
};


int main(int input)
{
	// If you want to instantiate a template,
	// you have to pass in the template parameters
	// between <...>
	IntArray<12> twelveArray;
	
	// Calling a templated method works just like
	// above. Be aware that 11 will be 
	// checked at compile time, so if you pass in a value
	// below 12, it will fail compilation.
	twelveArray.set<11>(18);
	
	return twelveArray.get(11);
}

