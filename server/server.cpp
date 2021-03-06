// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "Hello.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TPipeServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/server/TSimpleServer.h>

#include "../transport.h"

#include <iostream>
#include <chrono>
#include <thread>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using namespace ::hello;

class HelloHandler : virtual public HelloIf
{
public:
	HelloHandler() {}

	void echo(const std::string& msg)
	{
		printf("%s\n", msg.c_str());
	}

	int32_t add(const int32_t a, const int32_t b)
	{
		return a + b;
	}

	int32_t mul(const int32_t a, const int32_t b)
	{
		return a * b;
	}

	virtual void mulOfSum( MulOfSumOut& _return, const MulOfSumIn& input ) override
	{
		int mul = 1;
		_return.sums.reserve( input.pairs.size() );

		for ( const auto& p : input.pairs )
		{
			int s = p.a + p.b;
			//mul *= s;
			_return.sums.push_back( s );
		}
	}
};

class TMemoryBufferFactory : public TTransportFactory
{
public:
	TMemoryBufferFactory() {}
	virtual ~TMemoryBufferFactory() {}

	virtual stdcxx::shared_ptr<TTransport> getTransport(stdcxx::shared_ptr<TTransport> trans)
	{
		return stdcxx::make_shared<TMemoryBuffer>();
	}
};

int main(int argc, char** argv)
{
	/*windows_shared_memory shm(create_only, "shared_memory", read_write, 1024);
	mapped_region region(shm, read_write);

	char* mem = static_cast<char*>(region.get_address());
	std::memset(mem, 0, region.get_size());
	std::strcpy(mem, "hello, world");

	 while (*mem != 0) {
	 	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	 }

	 std::cout << "Read a 0 !\n";*/

	constexpr int a = 0;
	stdcxx::shared_ptr<TServerTransport> serverTransport;
	stdcxx::shared_ptr<TTransportFactory> transportFactory;
	if ( a == 1 )
	{
		serverTransport = stdcxx::make_shared<TServerSocket>( 4242 );
		transportFactory = stdcxx::make_shared<TBufferedTransportFactory>();
	}
	else if ( a == 2 )
	{
		serverTransport = stdcxx::make_shared<TPipeServer>( "my_pipe" );
		transportFactory = stdcxx::make_shared<TBufferedTransportFactory>();
	}
	else 
	{
		serverTransport = stdcxx::make_shared<SharedMemory::SharedMemoryServerTransport>();
		transportFactory = stdcxx::make_shared<TFramedTransportFactory>();
	} 

	auto handler   = stdcxx::make_shared<HelloHandler>();
	auto processor = stdcxx::make_shared<HelloProcessor>(handler);
	auto protocolFactory  = stdcxx::make_shared<TBinaryProtocolFactory>();

	TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
	server.serve();

	return 0;
}
