#ifndef SHARED_MEMORY_TRANSPORT
#define SHARED_MEMORY_TRANSPORT

#include <thrift/transport/TVirtualTransport.h>
#include <thrift/transport/TServerTransport.h>

#include <boost/interprocess/windows_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

namespace SharedMemory
{
static constexpr size_t DEFAULT_SIZE = 1 << 12;

struct SharedMemoryBuffer
{
	size_t len;
	size_t data_size;

	boost::interprocess::interprocess_semaphore client_data_avail, server_data_avail;

	SharedMemoryBuffer(size_t size = DEFAULT_SIZE);
};

class SharedMemoryTransport : public apache::thrift::transport::TVirtualTransport<SharedMemoryTransport>
{
	apache::thrift::stdcxx::shared_ptr<boost::interprocess::windows_shared_memory> shm;
	apache::thrift::stdcxx::shared_ptr<boost::interprocess::mapped_region>        ptr;

	bool server_side;
	bool consumed_all_data;

	size_t read_offset;
	size_t size;

public:
	SharedMemoryTransport(size_t size, bool server_side = false);
	virtual ~SharedMemoryTransport();
	uint32_t read(uint8_t* buf, uint32_t len);
	uint32_t readAll(uint8_t* buf, uint32_t len);
	void write(const uint8_t* buf, uint32_t len);

	const uint8_t* borrow(uint8_t* buf, uint32_t* len)
	{
		throw "SharedMemoryTransport::borrow() is not implemented.";
		return 0;
	}

	void consume(uint32_t len)
	{
		throw "SharedMemoryTransport::consume() is not implemented.";
	}

	virtual bool isOpen() override
	{
		throw "SharedMemoryTransport::isOpen() is not implemented.";
		return true;
	}

	virtual bool peek() override
	{
		throw "SharedMemoryTransport::peek() is not implemented.";
		return false;
	}

	virtual void open() override;
	virtual void close() override;
};

class SharedMemoryServerTransport : public apache::thrift::transport::TServerTransport
{
	apache::thrift::stdcxx::shared_ptr<boost::interprocess::windows_shared_memory> shm;
	apache::thrift::stdcxx::shared_ptr<boost::interprocess::mapped_region>         ptr;
	size_t size;

public:
	SharedMemoryServerTransport(size_t size = DEFAULT_SIZE);
	~SharedMemoryServerTransport();

	virtual void listen() override;
	virtual void interrupt() override;
	virtual void interruptChildren() override;

	virtual void close() override;

	virtual THRIFT_SOCKET getSocketFD() override;

protected:
	virtual apache::thrift::stdcxx::shared_ptr<apache::thrift::transport::TTransport> acceptImpl();
};
}

#endif // SHARED_MEMORY_TRANSPORT