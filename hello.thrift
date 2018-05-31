namespace cpp hello
namespace csharp hello

struct Pair {
	1: i32 a,
	2: i32 b,
}

struct MulOfSumIn {
	1: list<Pair> pairs,
}

struct MulOfSumOut {
	1: list<i32> sums,
	2: i64 mul,
}

service Hello {
	void echo(1: string msg)
	i32 add(1: i32 a, 2: i32 b);
	i32 mul(1: i32 a, 2: i32 b);

	MulOfSumOut mulOfSum(1: MulOfSumIn input);
}
