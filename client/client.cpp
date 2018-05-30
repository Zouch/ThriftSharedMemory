#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include <thrift/protocol/TBinaryProtocol.h>

#include <Hello.h>

#include <iostream>

using namespace hello;

using namespace boost::interprocess;

#include "../transport.cpp"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

int main()
{
	auto socket = stdcxx::make_shared<MyTransport>();
	// auto socket    = apache::thrift::stdcxx::make_shared<TSocket>("localhost", 4242);
	auto transport = apache::thrift::stdcxx::make_shared<TBufferedTransport>(socket);
	auto protocol  = apache::thrift::stdcxx::make_shared<TBinaryProtocol>(transport);
	auto client    = apache::thrift::stdcxx::make_shared<HelloClient>(protocol);

	std::cout << "Open transport\n";
	transport->open();
	std::cout << "Done\n";

	std::cout << "Echo hello\n";
	client->echo("hello");
	std::cout << "Done.\n";

	std::cout << "Add\n";
	std::cout << client->add(21, 21) << std::endl;
	std::cout << "Done\n";

	// shared_memory_object shm(open_only, "shared_memory", read_write);
	// mapped_region region(shm, read_write);

	// char* mem = static_cast<char*>(region.get_address());
	// std::cout << mem << std::endl;
	// *mem = 0;

	return 0;
}
