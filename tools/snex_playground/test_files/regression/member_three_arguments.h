/*
BEGIN_TEST_DATA
  f: test
  ret: int
  args: int
  input: 12
  output: 13
  error: ""
  filename: "regression/member_three_arguments"
END_TEST_DATA
*/

/** Setup: A member function has two explicit parameters and an object receiver.
 *  Scenario: Invoke it through an interpreter-owned struct instance.
 *  Expected: All three parameters reach the function in the correct order.
 */
struct Obj
{
	int value = 0;

	void set(int v, double v2)
	{
		value = v + (int)v2;
	}
};

int test(int input)
{
	Obj obj;
	obj.set(input, 1.0);
	return obj.value;
}
