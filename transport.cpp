#include <thrift/transport/TVirtualTransport.h>
#include <thrift/transport/TServerTransport.h>

#include <boost/interprocess/managed_shared_memory.hpp>

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

using namespace apache::thrift;
using namespace apache::thrift::transport;

using namespace boost::interprocess;

struct Header
{
	uint8_t status = 0;
	size_t len = 0;
};

class MyTransport : public TVirtualTransport<MyTransport>
{
	stdcxx::shared_ptr<shared_memory_object> shm;
	stdcxx::shared_ptr<mapped_region>        ptr;

	bool server_side;
	uint8_t value_to_wait;
	uint8_t value_to_write;

public:
	MyTransport(bool server_side = false)
		: server_side(server_side)
	{
		shm.reset(new shared_memory_object(open_only, "my_shared_memory", read_write));
		ptr.reset(new mapped_region(*shm, read_write));

		value_to_wait = server_side ? 1 : 2;
		value_to_write = server_side ? 2 : 1;
	}

	virtual ~MyTransport()
	{
		close();
	}

	uint32_t read(uint8_t* buf, uint32_t len)
	{
		Header* header = reinterpret_cast<Header*>(ptr->get_address());

		while ( header->status != value_to_wait )
		{
			// wait
			std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );
		}

		size_t data_len = header->len;
		void* data = header + sizeof( Header );
		std::memcpy(buf, data, data_len);
		header->status = 0;
		return data_len;
	}

	uint32_t readAll(uint8_t* buf, uint32_t len)
	{
		Header* header = reinterpret_cast<Header*>(ptr->get_address());

		while ( header->status != value_to_wait )
		{
			// wait
			std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );
		}

		size_t data_len = header->len;
		void* data = header + sizeof( Header );
		std::memcpy( buf, data, len );
		header->status = 0;
		return len;
	}

	void write(const uint8_t* buf, uint32_t len)
	{
		Header* header = reinterpret_cast<Header*>(ptr->get_address());
		void* data = header + sizeof( Header );

		std::memcpy(data, buf, len);

		header->len = len;
		header->status = value_to_write;
	}

	const uint8_t* borrow(uint8_t* buf, uint32_t* len)
	{
		return 0;
	}

	void consume(uint32_t len) {}

	virtual bool isOpen() override
	{
		return true;
	}

	virtual bool peek() override
	{
		return false;
	}

	virtual void open() override
	{
		named_condition cond(open_only, "my_condition");
		cond.notify_one();
	}

	virtual void close() override {
		Header* header = reinterpret_cast<Header*>(ptr->get_address());
		header->status = value_to_write;
		header->len = 0;
	}
};

class MyServerTransport : public TServerTransport
{
	stdcxx::shared_ptr<shared_memory_object> shm;
	stdcxx::shared_ptr<mapped_region>        ptr;

public:
	MyServerTransport()
	{
		shm.reset(new shared_memory_object(open_or_create, "my_shared_memory", read_write));
		shm->truncate(1 << 16);
		ptr.reset(new mapped_region(*shm, read_write));
		memset( ptr->get_address(), 0, 1 << 16 );
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