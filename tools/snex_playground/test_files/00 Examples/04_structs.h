/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 33
  error: ""
  filename: "00 Examples/04_structs"
END_TEST_DATA
*/

/** This test introduces structs which are
    complex types that can have methods operate
    on their data
*/
struct MyFirstStruct
{
	
	// You can define nested structures
	struct InnerStruct
	{
		int i1 = 0;
		int i2 = 1;
	};

	/* A method that has the same name as the struct is called "Constructor"
	   and will be called whenever the object is constructed.
	*/
	MyFirstStruct(int initValue)
	{
		is1.i1 = initValue * 2;
	}

	// Add a native data type member.
	int m1 = 9;

	// Create members of the inner struct type.
	InnerStruct is1;
	InnerStruct is2;	

	// Add a simple method that returns the data member
	// const means that the method will not change the struct
	// and gives the compiler possibilities for optimization
	int getM1() const
	{
		return m1;
	}	
}; // <= never forget the semicolon at the end...


/* You can declare global objects. In this case, the 19 will be passed to 
   the constructor, while the other data values will be set to their default. 
*/
MyFirstStruct obj1;

int main(int input)
{
	// You can also declare local objects. These will be allocated on the function
	// stack with no memory heap allocation messing with realtime performance!
	MyFirstStruct localObject(input);
	
	// Uncomment this line and you will see a
	// dump of the memory layout of this object
	//Console.print(localObject);
	
	return localObject.is1.i1 + obj1.getM1();
}

