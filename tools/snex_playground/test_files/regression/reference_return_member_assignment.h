/*
BEGIN_TEST_DATA
  f: test
  ret: int
  args: int
  input: 15
  output: 15
  error: ""
  filename: "regression/reference_return_member_assignment"
END_TEST_DATA
*/

/** Setup: A function returns a reference to a global struct.
 *  Scenario: Assign to a member through the returned reference.
 *  Expected: Reading the original struct returns the assigned value.
 */
struct X
{
	int value = 90;
};

X obj;

X& getX()
{
	return obj;
}

int test(int input)
{
	getX().value = input;
	return obj.value;
}
