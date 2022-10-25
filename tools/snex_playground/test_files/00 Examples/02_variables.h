/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "00 Examples/02_variables"
END_TEST_DATA
*/

/** This test contains an overview over 
    different variables and shows all 
    native types supported by SNEX.
*/


// ## Global variables
// variables that are visible in the entire file

int x = 12; // an integer number

float y = 12.5f; // a floating point number
                 // (note the `f` suffix)
                 
double z = 12.9; // a double precision floating point number



int main(int input)
{
	// ## Local variables
	// variables that are visible in the scope that 
	// they are defined
	double z_ = 13.9;
	
	// Change the global variable
	y = 18.0f;
	
	// See how a type mismatch produces a warning
	// hover over the yellow line to see the warning
	// (Implicit cast means that you convert the number
	// to the desired type which might result in data loss
	// eg. if you want to assign 9.5 to an integer)
	int x_ = 9.5;
	
	
	// Be a good boy and return a value that passes the test
	return x;
}

