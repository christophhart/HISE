/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "00 Examples/01_hello_world"
END_TEST_DATA
*/

/** Hello world - the first test

	These files contain tests that are executed
	and ensure correct functionality, as well as
	try to cover as much of the feature set as 
	possible. Since they are executed on each
	unit test run, it's very likely that these
	test will never be outdated, so they also 
	can be used as playground to check out language
	features.
	
	A valid test must consist of two things:
	
	1. A metadata comment at the beginning of the file
	   (see above)
	2. A main function with the test procedure
	   (see below)
	   
	For a detailed explanation of how the test 
	metadata is parsed, take a look at the Readme.md
	file in the root directory of the test.
*/

/** The test procedure with the prototype as described in the metadata. */
int main(int input)
{
	// We return 12, so it matches the `output` of the metadata
	// and the test passes. Try changing the value and see
	// what happens if the test fails.
	return 12;
}

