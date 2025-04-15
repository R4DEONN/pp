#include <boost/asio.hpp>

class UDPSocket
{
public:
	using udp = boost::asio::ip::udp;

	UDPSocket(boost::asio::io_context& io_context)
		: m_socket(io_context)
	{
	}

	void Bind(int port)
	{
		udp::endpoint endpoint(udp::v4(), port);
		m_socket.open(endpoint.protocol());
		m_socket.bind(endpoint);
	}

	void SendTo(const std::string& address, int port, const void* data, size_t size)
	{
		udp::endpoint endpoint(boost::asio::ip::make_address(address), port);
		m_socket.send_to(boost::asio::buffer(data, size), endpoint);
	}

	size_t Receive(void* buffer, size_t size, std::string* senderIp = nullptr, int* senderPort = nullptr)
	{
		udp::endpoint sender_endpoint;
		size_t received = m_socket.receive_from(
			boost::asio::buffer(buffer, size), sender_endpoint);

		if (senderIp)
		{
			*senderIp = sender_endpoint.address().to_string();
		}
		if (senderPort)
		{
			*senderPort = sender_endpoint.port();
		}

		return received;
	}

	udp::socket& GetSocket()
	{
		return m_socket;
	}

private:
	udp::socket m_socket;
};