/*
BEGIN_TEST_DATA
  f: test
  ret: int
  args: int
  input: 12
  output: 1
  error: ""
  filename: "regression/missing_memory_reference"
END_TEST_DATA
*/

int count = 0;

struct Child
{
	~Child()
	{
		count++;
	}
};

struct Outer
{
	Child member;
};

int test(int input)
{
	Outer value;
	return 1;
}
