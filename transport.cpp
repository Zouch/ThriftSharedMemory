#include <thrift/transport/TVirtualTransport.h>
#include <thrift/transport/TServerTransport.h>

#include <boost/interprocess/windows_shared_memory.hpp>

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

#include <array>

using namespace apache::thrift;
using namespace apache::thrift::transport;

using namespace boost::interprocess;

static constexpr size_t MAX_SIZE = 1 << 16;

struct SharedMemoryBuffer
{
	size_t len = 0;

	boost::interprocess::interprocess_semaphore client_data_avail, server_data_avail;

	std::array<uint8_t, MAX_SIZE> data;

	SharedMemoryBuffer()
		: len(0) 
		, client_data_avail( 0 )
		, server_data_avail( 0 )
	{
		memset( data.data(), 0, MAX_SIZE );
	}
};

class MyTransport : public TVirtualTransport<MyTransport>
{
	stdcxx::shared_ptr<windows_shared_memory> shm;
	stdcxx::shared_ptr<mapped_region>        ptr;

	bool server_side;
	bool consumed_all_data;

	size_t read_offset;

public:
	MyTransport(bool server_side = false)
		: server_side(server_side)
		, consumed_all_data(true)
		, read_offset(0)
	{
		shm.reset(new windows_shared_memory(open_only, "my_shared_memory", read_write));
		ptr.reset(new mapped_region(*shm, read_write));
	}

	virtual ~MyTransport()
	{
		close();
	}

	uint32_t read(uint8_t* buf, uint32_t len)
	{
		SharedMemoryBuffer* data = reinterpret_cast<SharedMemoryBuffer*>(ptr->get_address());

		if ( consumed_all_data )
		{
			if ( server_side ) data->client_data_avail.wait();
			else data->server_data_avail.wait();
		}

		size_t data_len = data->len;
		const uint8_t* read_ptr = nullptr;
		if ( data_len > len )
		{
			consumed_all_data = false;
			// We need to read less data than available

			// FIXME(CMA): Do we want to copy the data locally, or is reading from the shared memory fine ? 
			// TODO(CMA): Check if this pointer reset is fine
			if ( read_offset + len > data_len )
			{
				read_offset = 0;
			}

			read_ptr = data->data.data() + read_offset;
			read_offset += len;
			data_len = len;

			if ( read_offset == data->len )
			{
				consumed_all_data = true;
			}
		} 
		else
		{
			read_ptr = data->data.data();
		}

		std::memcpy(buf, read_ptr, data_len);

		return data_len;
	}

	uint32_t readAll(uint8_t* buf, uint32_t len)
	{
		SharedMemoryBuffer* data = reinterpret_cast<SharedMemoryBuffer*>(ptr->get_address());

		if ( consumed_all_data )
		{
			if ( server_side ) data->client_data_avail.wait();
			else data->server_data_avail.wait();
		}

		//size_t data_len = data->len;
		const uint8_t* read_ptr = nullptr;
		
		read_ptr = data->data.data() + read_offset;
		read_offset = 0;

		std::memcpy( buf, read_ptr, len );

		consumed_all_data = true;

		return len;
	}

	void write(const uint8_t* buf, uint32_t len)
	{
		//std::cout << (server_side ? "Server" : "Client") << " start write()\n";
		SharedMemoryBuffer* data = reinterpret_cast<SharedMemoryBuffer*>(ptr->get_address());
		std::memcpy(data->data.data(), buf, len);
		data->len = len;
		if ( server_side ) data->server_data_avail.post();
		else data->client_data_avail.post();
		//std::cout << (server_side ? "Server" : "Client") << " done write()\n";
	}

	const uint8_t* borrow(uint8_t* buf, uint32_t* len)
	{
		std::cout << "MyTransport::borrow()\n";
		return 0;
	}

	void consume(uint32_t len) {
		std::cout << "MyTransport::consume()\n";
	}

	virtual bool isOpen() override
	{
		std::cout << "MyTransport::isOpen()\n";
		return true;
	}

	virtual bool peek() override
	{
		std::cout << "MyTransport::peek()\n";
		return false;
	}

	virtual void open() override
	{
		named_condition cond(open_only, "my_condition");
		cond.notify_one();
	}

	virtual void close() override {
		SharedMemoryBuffer* data = reinterpret_cast<SharedMemoryBuffer*>(ptr->get_address());
		data->len = 0;
		if ( !server_side ) data->client_data_avail.post();
	}
};

class MyServerTransport : public TServerTransport
{
	stdcxx::shared_ptr<windows_shared_memory> shm;
	stdcxx::shared_ptr<mapped_region>        ptr;

public:
	MyServerTransport()
	{
		size_t size = sizeof( SharedMemoryBuffer );
		shm.reset(new windows_shared_memory(open_or_create, "my_shared_memory", read_write, size));
		ptr.reset(new mapped_region(*shm, read_write));

		SharedMemoryBuffer* data = new (ptr->get_address()) SharedMemoryBuffer;
	}

	~MyServerTransport()
	{
		shared_memory_object::remove("my_shared_memory");
	}

	virtual void listen() override
	{
		std::cout << "MyServerTransport::listen()\n";
		TServerTransport::listen();
	}

	virtual void interrupt() override
	{
		std::cout << "MyServerTransport::interrupt()\n";
		TServerTransport::interrupt();
	}

	virtual void interruptChildren() override
	{
		std::cout << "MyServerTransport::interruptChildren()\n";
		TServerTransport::interruptChildren();
	}

	virtual void close() override
	{
		std::cout << "MyServerTransport::close()\n";
	}

	virtual THRIFT_SOCKET getSocketFD() override
	{
		std::cout << "MyServerTransport::getSocketFD()\n";
		return TServerTransport::getSocketFD();
	}

protected:
	virtual stdcxx::shared_ptr<TTransport> acceptImpl()
	{
		named_mutex::remove("my_mutex");
		named_condition::remove("my_condition");

		named_condition cond(create_only, "my_condition");
		named_mutex     mutex(create_only, "my_mutex");

		scoped_lock<named_mutex> lock(mutex);
		cond.wait(lock);

		named_mutex::remove("my_mutex");
		named_condition::remove("my_condition");

		return std::make_shared<MyTransport>(true);
	}
};

class SharedMemoryTransportFactory : public TTransportFactory
{
};