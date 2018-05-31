#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/transport/TPipe.h>

#include <thrift/protocol/TBinaryProtocol.h>

#include <Hello.h>

#include "../transport.h"

#include <iostream>
#include <chrono>
#include <functional>

using namespace hello;

using namespace boost::interprocess;

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

int main()
{
	constexpr int a = 0;
	stdcxx::shared_ptr<TTransport> socket;
	stdcxx::shared_ptr<TTransport> transport;
	if ( a == 1 )
	{
		socket = stdcxx::make_shared<TSocket>( "localhost", 4242 );
		transport = stdcxx::make_shared<TBufferedTransport>( socket );
	}
	else if ( a == 2 )
	{
		socket = stdcxx::make_shared<TPipe>( "my_pipe" );
		transport = stdcxx::make_shared<TBufferedTransport>( socket );
	}
	else
	{
		socket = stdcxx::make_shared<SharedMemory::SharedMemoryTransport>(SharedMemory::DEFAULT_SIZE);
		transport = stdcxx::make_shared<TFramedTransport>( socket );
	}
	
	auto protocol  = stdcxx::make_shared<TBinaryProtocol>(transport);
	auto client    = stdcxx::make_shared<HelloClient>(protocol);

	transport->open();
	
	const size_t count = 100000;
	const size_t reps = 100;
	double total = 0.0;

	//for ( int i = 1; i <= reps; ++i )
	int i = 1;
	{
		std::vector<Pair> a(i);
		MulOfSumIn in;
		MulOfSumOut out;

		double frame_t = 0.0;
		std::vector<double> times;

		for ( size_t j = 0u; j < count; ++j )
		{
			Pair p;
			p.a = p.b = j + 1;
			std::fill( a.begin(), a.end(), p );

			in.pairs = a;
			auto beg = std::chrono::high_resolution_clock::now();
			client->mulOfSum( out, in );
			auto end = std::chrono::high_resolution_clock::now();
			const std::chrono::duration<double> elapsed = end - beg;
			const double t = elapsed.count();

			frame_t += t;

			times.push_back( t );
		}

		std::sort( times.begin(), times.end(), std::less<double>() );
		double min_t = times.front();
		double max_t = times.back();

		double median = times[times.size() / 2];

		total += frame_t;
		std::cout << sizeof( Pair ) * a.size() << " " << (frame_t / count) * 1e6 << " " << median * 1e6 << " " << min_t * 1e6 << " " << max_t * 1e6 << std::endl;
	}

	std::cout << "Done in " << total << "s. Average time is " << (total / (count * reps)) * 1e6 << "us\n";

	// shared_memory_object shm(open_only, "shared_memory", read_write);
	// mapped_region region(shm, read_write);

	// char* mem = static_cast<char*>(region.get_address());
	// std::cout << mem << std::endl;
	// *mem = 0;

	return 0;
}
