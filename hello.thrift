namespace cpp hello
namespace csharp hello

service Hello {
	void echo(1: string msg)
	i32 add(1: i32 a, 2: i32 b);
	i32 mul(1: i32 a, 2: i32 b);
}
