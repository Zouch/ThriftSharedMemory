#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include <thrift/protocol/TBinaryProtocol.h>

#include <Hello.h>

#include <iostream>
#include <chrono>

using namespace hello;

using namespace boost::interprocess;

#include "../transport.cpp"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

int main()
{
	constexpr int a = 0;
	stdcxx::shared_ptr<TTransport> socket;
	stdcxx::shared_ptr<TTransport> transport;
	if ( a )
	{
		socket = stdcxx::make_shared<TSocket>( "localhost", 4242 );
		transport = stdcxx::make_shared<TBufferedTransport>( socket );
	}
	else
	{
		socket = stdcxx::make_shared<MyTransport>();
		transport = stdcxx::make_shared<TFramedTransport>( socket );
	}
	
	auto protocol  = stdcxx::make_shared<TBinaryProtocol>(transport);
	auto client    = stdcxx::make_shared<HelloClient>(protocol);

	transport->open();
	
	const size_t count = 1000;
	const size_t reps = 100;
	double total = 0.0;

	for ( int i = 1; i <= reps; ++i )
	{
		std::vector<Pair> a(i * 10);
		MulOfSumIn in;
		MulOfSumOut out;

		auto beg = std::chrono::high_resolution_clock::now();
		for ( size_t j = 0u; j < count; ++j )
		{
			Pair p;
			p.a = p.b = j + 1;
			std::fill( a.begin(), a.end(), p );

			in.pairs = a;
			client->mulOfSum( out, in );
		}
		auto end = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double> elapsed = end - beg;
		const double t = elapsed.count();
		total += t;
		std::cout << "Elapsed time for " << count << " requests: " << t << "s (" << (t / count) * 1e6 << "us average). " << sizeof(Pair) * a.size() << " bytes of data\n";
	}

	std::cout << "Done in " << total << "s. Average time is " << (total / (count * reps)) * 1e6 << "us\n";

	// shared_memory_object shm(open_only, "shared_memory", read_write);
	// mapped_region region(shm, read_write);

	// char* mem = static_cast<char*>(region.get_address());
	// std::cout << mem << std::endl;
	// *mem = 0;

	return 0;
}
