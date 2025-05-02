#pragma once

#include "Constants.h"
#include "PacketHeader.h"
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/opencv.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <vector>
#include <thread>
#include <SDL2/SDL.h>

class Server
{
public:
	explicit Server(unsigned short port)
		: m_socket(m_io_context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port)),
		  m_connectionHandler(&Server::HandleClientConnections, this)
	{
		StartReceive();
		std::cout << "Server started on port: " << port << std::endl;
	}

	~Server()
	{

		if (m_audio_running)
		{
			SDL_CloseAudioDevice(m_audio_device);
			SDL_Quit();
		}
		m_running = false;
		m_io_context.stop();
		if (m_connectionHandler.joinable())
		{
			m_connectionHandler.join();
		}
	}

	void Run()
	{
		cv::namedWindow(APP_TITLE, cv::WINDOW_AUTOSIZE);
		cv::VideoCapture cap(0, cv::CAP_DSHOW);
		if (!cap.isOpened())
		{
			std::cerr << "Error: Could not open camera." << std::endl;
			return;
		}
		HandleAudio();

		cv::Mat frame;

		while (true)
		{
			cap >> frame;
			if (frame.empty())
			{
				std::cerr << "Error: Captured frame is empty." << std::endl;
				break;
			}

			cv::flip(frame, frame, 1);

			SendFrameToClients(frame);

			cv::imshow(APP_TITLE, frame);

			int key = cv::waitKey(30);
			if (key == 27)
			{
				break;
			}

			if (!IsWindowOpen())
			{
				break;
			}
		}

		m_running = false;
	}

private:
	static bool IsWindowOpen()
	{
		return cv::getWindowProperty(APP_TITLE, cv::WND_PROP_VISIBLE) >= 1;
	}

	void SendFrameToClients(const cv::Mat& frame)
	{
		std::vector<uchar> buffer;
		std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 80};
		cv::imencode(".jpg", frame, buffer, params);

		PacketHeader header;
		header.type = 0;
		header.size = buffer.size();

		std::lock_guard<std::mutex> lock(m_mutex);
			std::vector<uint8_t> packet(sizeof(PacketHeader) + buffer.size());
			memcpy(packet.data(), &header, sizeof(PacketHeader));
			memcpy(packet.data() + sizeof(PacketHeader), buffer.data(), buffer.size());

		for (auto it = m_clients.begin(); it != m_clients.end(); )
		{
			try
			{
				m_socket.send_to(boost::asio::buffer(packet), *it);
				++it;
			}
			catch (const boost::system::system_error& e)
			{
				std::cerr << "Client disconnected: " << e.what() << std::endl;
				it = m_clients.erase(it);
			}
		}
	}

	void HandleClientConnections()
	{
		m_io_context.run();
	}

	static void AudioCallback(void* userdata, Uint8* stream, int len)
	{
		Server* server = static_cast<Server*>(userdata);
		if (!server || !server->m_audio_running)
		{
			return;
		}

		std::vector<float> audio_buffer(len / sizeof(float));
		memcpy(audio_buffer.data(), stream, len);
		server->SendAudioToClients(audio_buffer);
	}

	void SendAudioToClients(const std::vector<float>& audio_data)
	{
		PacketHeader header{};
		header.type = 1; // Audio
		header.size = audio_data.size() * sizeof(float);

		std::vector<uint8_t> packet(sizeof(PacketHeader) + header.size);
		memcpy(packet.data(), &header, sizeof(PacketHeader));
		memcpy(packet.data() + sizeof(PacketHeader), audio_data.data(), header.size);

		std::lock_guard<std::mutex> lock(m_mutex);
		for (auto it = m_clients.begin(); it != m_clients.end(); )
		{
			try
			{
				m_socket.send_to(boost::asio::buffer(packet), *it);
				++it;
			}
			catch (const boost::system::system_error& e)
			{
				std::cerr << "Client disconnected: " << e.what() << std::endl;
				it = m_clients.erase(it);
			}
		}
	}

	void StartReceive()
	{
		boost::asio::ip::udp::endpoint client_endpoint;
		char buffer[1];
		m_socket.async_receive_from(
			boost::asio::buffer(buffer),
			client_endpoint,
			[this, &client_endpoint](const boost::system::error_code& error, std::size_t bytes_transferred)
			{
				if (!error && m_running)
				{
					std::lock_guard<std::mutex> lock(m_mutex);
					if (std::find(m_clients.begin(), m_clients.end(), client_endpoint) == m_clients.end())
					{
						m_clients.push_back(client_endpoint);
						std::cout << "New client connected: " << client_endpoint << std::endl;
					}
					StartReceive();
				}
			});
	}

	void HandleAudio()
	{
		if (SDL_Init(SDL_INIT_AUDIO) < 0)
		{
			std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
			m_audio_running = false;
			return;
		}

		SDL_AudioSpec desiredSpec, obtainedSpec;
		desiredSpec.freq = 44100;
		desiredSpec.format = AUDIO_F32;
		desiredSpec.channels = 1;
		desiredSpec.samples = 1024;
		desiredSpec.callback = AudioCallback;
		desiredSpec.userdata = this;

		m_audio_device = SDL_OpenAudioDevice(NULL, 1, &desiredSpec, &obtainedSpec, 0);
		if (!m_audio_device)
		{
			std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
			m_audio_running = false;
			SDL_Quit();
			return;
		}

		SDL_PauseAudioDevice(m_audio_device, 0);
	}

	boost::asio::io_context m_io_context;
	boost::asio::ip::udp::socket m_socket;
	std::vector<boost::asio::ip::udp::endpoint> m_clients;
	std::thread m_connectionHandler;
	std::thread m_audioHandler;
	std::atomic<bool> m_running = true;

	SDL_AudioDeviceID m_audio_device;
	bool m_audio_running = true;

	std::mutex m_mutex;
};