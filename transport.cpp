#include "transport.h"

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <iostream>

using namespace SharedMemory;

using namespace apache::thrift;
using namespace apache::thrift::transport;

using namespace boost::interprocess;

#define array_addr(d) (reinterpret_cast<uint8_t*>(d + sizeof(SharedMemoryBuffer)))

SharedMemoryBuffer::SharedMemoryBuffer(size_t size)
	: len(0)
	, data_size(size)
	, client_data_avail(0)
	, server_data_avail(0)
{
	//data = (uint8_t*)this + offsetof(SharedMemoryBuffer, data);
	//memset(data, 0, size);
}

SharedMemoryTransport::SharedMemoryTransport(size_t size, bool server_side)
	: server_side(server_side)
	, consumed_all_data(true)
	, read_offset(0)
	, size(size)
{
	shm.reset(new windows_shared_memory(open_only, "my_shared_memory", read_write));
	ptr.reset(new mapped_region(*shm, read_write));
}

SharedMemoryTransport::~SharedMemoryTransport()
{
	close();
}

uint32_t SharedMemoryTransport::read(uint8_t* buf, uint32_t len)
{
	SharedMemoryBuffer* data = reinterpret_cast<SharedMemoryBuffer*>(ptr->get_address());

	if (consumed_all_data)
	{
		if (server_side) data->client_data_avail.wait();
		else data->server_data_avail.wait();
	}

	size_t data_len = data->len;
	const uint8_t* read_ptr = nullptr;
	if (data_len > len)
	{
		consumed_all_data = false;
		// We need to read less data than available

		// FIXME(CMA): Do we want to copy the data locally, or is reading from the shared memory fine ? 
		// TODO(CMA): Check if this pointer reset is fine
		if (read_offset + len > data_len)
		{
			read_offset = 0;
		}

		read_ptr = array_addr(data) + read_offset;
		read_offset += len;
		data_len = len;

		if (read_offset == data->len)
		{
			consumed_all_data = true;
		}
	}
	else
	{
		read_ptr = array_addr(data);
	}

	std::memcpy(buf, read_ptr, data_len);

	return data_len;
}

uint32_t SharedMemoryTransport::readAll(uint8_t* buf, uint32_t len)
{
	SharedMemoryBuffer* data = reinterpret_cast<SharedMemoryBuffer*>(ptr->get_address());

	if (consumed_all_data)
	{
		if (server_side) data->client_data_avail.wait();
		else data->server_data_avail.wait();
	}

	//size_t data_len = data->len;
	const uint8_t* read_ptr = nullptr;

	read_ptr = array_addr(data) + read_offset;
	read_offset = 0;

	std::memcpy(buf, read_ptr, len);

	consumed_all_data = true;

	return len;
}

void SharedMemoryTransport::write(const uint8_t* buf, uint32_t len)
{
	//std::cout << (server_side ? "Server" : "Client") << " start write()\n";
	SharedMemoryBuffer* data = reinterpret_cast<SharedMemoryBuffer*>(ptr->get_address());
	std::memcpy(array_addr(data), buf, len);
	data->len = len;
	if (server_side) data->server_data_avail.post();
	else data->client_data_avail.post();
	//std::cout << (server_side ? "Server" : "Client") << " done write()\n";
}

void SharedMemoryTransport::open()
{
	named_condition cond(open_only, "my_condition");
	cond.notify_one();
}

void SharedMemoryTransport::close()
{
	SharedMemoryBuffer* data = reinterpret_cast<SharedMemoryBuffer*>(ptr->get_address());
	data->len = 0;
	if (!server_side) data->client_data_avail.post();
}

SharedMemoryServerTransport::SharedMemoryServerTransport(size_t size)
	: size(size)
{
	size_t total_size = sizeof(SharedMemoryBuffer) + size;
	shm.reset(new windows_shared_memory(open_or_create, "my_shared_memory", read_write, total_size));
	ptr.reset(new mapped_region(*shm, read_write));

	uint8_t* addr = reinterpret_cast<uint8_t*>(ptr->get_address());
	SharedMemoryBuffer* data = new (addr) SharedMemoryBuffer;
	memset(array_addr(addr), 0, size);
}

SharedMemoryServerTransport::~SharedMemoryServerTransport()
{
	shared_memory_object::remove("my_shared_memory");
}

void SharedMemoryServerTransport::listen()
{
	std::cout << "SharedMemoryServerTransport::listen()\n";
	TServerTransport::listen();
}

void SharedMemoryServerTransport::interrupt()
{
	std::cout << "SharedMemoryServerTransport::interrupt()\n";
	TServerTransport::interrupt();
}

void SharedMemoryServerTransport::interruptChildren()
{
	std::cout << "SharedMemoryServerTransport::interruptChildren()\n";
	TServerTransport::interruptChildren();
}

void SharedMemoryServerTransport::close()
{
	std::cout << "SharedMemoryServerTransport::close()\n";
}

THRIFT_SOCKET SharedMemoryServerTransport::getSocketFD()
{
	std::cout << "SharedMemoryServerTransport::getSocketFD()\n";
	return TServerTransport::getSocketFD();
}

stdcxx::shared_ptr<TTransport> SharedMemoryServerTransport::acceptImpl()
{
	named_mutex::remove("my_mutex");
	named_condition::remove("my_condition");

	named_condition cond(create_only, "my_condition");
	named_mutex     mutex(create_only, "my_mutex");

	scoped_lock<named_mutex> lock(mutex);
	cond.wait(lock);

	named_mutex::remove("my_mutex");
	named_condition::remove("my_condition");

	return std::make_shared<SharedMemoryTransport>(size, true);
}
