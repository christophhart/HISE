/*
BEGIN_TEST_DATA
  f: test
  ret: int
  args: int
  input: 15
  output: 30
  error: ""
  filename: "regression/reference_return_value_read"
END_TEST_DATA
*/

/** Setup: A function returns an int reference to a global value.
 *  Scenario: Assign through the reference, then read it as a value.
 *  Expected: Both reads observe the assigned integer, not its address.
 */
int x = 90;

int& getRef()
{
	return x;
}

int test(int input)
{
	getRef() = input;
	return (int)getRef() + x;
}
