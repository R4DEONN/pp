#pragma once

#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/opencv.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <vector>

//TODO: Добавить буферизацию звука
//TODO: 
class Client
{
public:
	Client(const std::string& server_ip, unsigned short port)
		: m_socket(m_io_context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0)),
		  m_server_endpoint(boost::asio::ip::make_address(server_ip), port)
	{
		m_socket.send_to(boost::asio::buffer(""), m_server_endpoint);
		std::cout << "Client connected to server: " << server_ip << ":" << port << std::endl;
	}

	~Client()
	{
		if (m_audio_running)
		{
			SDL_CloseAudioDevice(m_audio_device);
			SDL_Quit();
		}
	}

	void Run()
	{
		cv::namedWindow("Client Window", cv::WINDOW_AUTOSIZE);
		InitSDL();

		while (true)
		{
			std::vector<uint8_t> buffer(65536);
			boost::asio::ip::udp::endpoint sender_endpoint;
			size_t length = m_socket.receive_from(boost::asio::buffer(buffer), sender_endpoint);


			if (length > 0)
			{
				PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer.data());
				std::cout << "Received packet: type=" << static_cast<int>(header->type)
						  << ", size=" << header->size << std::endl;
				uint8_t* data = buffer.data() + sizeof(PacketHeader);

				if (header->type == 0) // Video
				{
					cv::Mat frame = cv::imdecode(cv::Mat(header->size, 1, CV_8UC1, data), cv::IMREAD_COLOR);
					if (!frame.empty())
					{
						cv::imshow("Client Window", frame);
					}
				}
				else if (header->type == 1) // Audio
				{
					const float* audio_data = reinterpret_cast<const float*>(data);
					size_t sample_count = header->size / sizeof(float);
					m_audio_buffer.insert(m_audio_buffer.end(), audio_data, audio_data + sample_count);
				}
			}

			int key = cv::waitKey(30);
			if (key == 27 || cv::getWindowProperty("Client Window", cv::WND_PROP_VISIBLE) < 1)
			{
				break;
			}
		}
	}

private:
	boost::asio::io_context m_io_context;
	boost::asio::ip::udp::socket m_socket;
	boost::asio::ip::udp::endpoint m_server_endpoint;
	SDL_AudioDeviceID m_audio_device;
	bool m_audio_running = true;
	std::vector<float> m_audio_buffer;

	static void AudioPlaybackCallback(void* userdata, Uint8* stream, int len)
	{
		Client* client = static_cast<Client*>(userdata);
		if (!client || !client->m_audio_running)
		{
			return;
		}

		float* output = reinterpret_cast<float*>(stream);
		size_t available_samples = client->m_audio_buffer.size();

		size_t samples_to_copy = std::min(static_cast<size_t>(len / sizeof(float)), available_samples);
		std::copy(client->m_audio_buffer.begin(), client->m_audio_buffer.begin() + samples_to_copy, output);

		client->m_audio_buffer.erase(client->m_audio_buffer.begin(),
			client->m_audio_buffer.begin() + samples_to_copy);
	}

	void InitSDL()
	{
		if (SDL_Init(SDL_INIT_AUDIO) < 0)
		{
			std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
			m_audio_running = false;
		}
		else
		{
			SDL_AudioSpec desiredSpec, obtainedSpec;
			desiredSpec.freq = 44100;
			desiredSpec.format = AUDIO_F32;
			desiredSpec.channels = 1;
			desiredSpec.samples = 1024;
			desiredSpec.callback = AudioPlaybackCallback;
			desiredSpec.userdata = this;

			m_audio_device = SDL_OpenAudioDevice(NULL, 0, &desiredSpec, &obtainedSpec, 0);
			if (!m_audio_device)
			{
				std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
				m_audio_running = false;
			}
			else
			{
				SDL_PauseAudioDevice(m_audio_device, 0);
			}
		}

	}
};