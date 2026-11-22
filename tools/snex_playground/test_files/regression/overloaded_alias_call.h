// Run with --backend:interpreter. Expected: --args:1 returns 9; --args:4 returns 3.
int get(double input) { return 3; }
int get(float input) { return 9; }

int test(int input)
{
	using T = float;
	T v = (T)input;
	if (input > 2)
	{
		using T = double;
		T inner = (T)input;
		return get(inner);
	}
	return get(v);
}
