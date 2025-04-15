#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <chrono>
#include "UDPSocket.h"
#include <memory>
#include <atomic>
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/opencv.hpp>

using namespace std::chrono_literals;

void RunStationMode(int port)
{
	try
	{
		boost::asio::io_context io_context;
		UDPSocket socket(io_context);
		socket.Bind(port);

//		VideoCapturer videoCapturer;
//		AudioCapturer audioCapturer;

		std::vector<std::pair<std::string, int>> clients;
		uint32_t sequenceNum = 0;

		while (true)
		{
			auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count();

//			auto audioData = audioCapturer.getAudioData();
//			if (!audioData.empty()) {
//				auto packet = PacketBuilder::buildAudioPacket(audioData, timestamp, sequenceNum++);
//
//				// Отправляем всем клиентам
//				for (const auto& client : clients) {
//					socket.sendTo(client.first, client.second, packet.data(), packet.size());
//				}
//			}

			// Захватываем видео
//			cv::Mat frame;
//			if (videoCapturer.captureFrame(frame)) {
//				// Динамически регулируем качество видео
//				int quality = clients.empty() ? 50 : std::max(10, 50 - static_cast<int>(clients.size() * 5));
//
//				auto packet = PacketBuilder::buildVideoPacket(frame, timestamp, sequenceNum++, quality);
//
//				// Отправляем всем клиентам
//				for (const auto& client : clients) {
//					socket.sendTo(client.first, client.second, packet.data(), packet.size());
//				}
//			}

			// Проверяем новые подключения
			char buffer[1024];
			std::string senderIp;
			int senderPort;
			if (socket.Receive(buffer, sizeof(buffer), &senderIp, &senderPort) > 0)
			{
				auto it = std::find_if(clients.begin(), clients.end(),
					[&](const auto& c)
					{ return c.first == senderIp && c.second == senderPort; });

				if (it == clients.end())
				{
					clients.emplace_back(senderIp, senderPort);
					std::cout << "New client connected: " << senderIp << ":" << senderPort << std::endl;
				}
			}

			io_context.poll();

			std::this_thread::sleep_for(1ms);
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Station error: " << e.what() << std::endl;
	}
}

void RunReceiverMode(const std::string& address, int port)
{
	try
	{
		boost::asio::io_context io_context;
		UDPSocket socket(io_context);
		socket.Bind(0);

		char hello = 0;
		socket.SendTo(address, port, &hello, 1);

		cv::namedWindow("Webcam", cv::WINDOW_AUTOSIZE);

		cv::VideoCapture cap(0);
		cv::Mat originalImage;
		cv::Mat mirroredImage;

		while (true)
		{
			cap >> originalImage;
			cv::flip(originalImage, originalImage, 1);
			cv::imshow("Webcam", originalImage);
			int key = cv::waitKey(30);
			if (key == 27)
			{
				break;
			}

			if (cv::getWindowProperty("Webcam", cv::WND_PROP_VISIBLE) < 1)
			{
				break;
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Receiver error: " << e.what() << std::endl;
	}
}

int main(int argc, char* argv[])
{
	try
	{
		if (argc == 2)
		{
			int port = std::stoi(argv[1]);
			RunStationMode(port);
		}
		else if (argc == 3)
		{
			std::string address = argv[1];
			int port = std::stoi(argv[2]);
			RunReceiverMode(address, port);
		}
		else
		{
			std::cerr << "Usage:\n"
					  << "  Station mode: tv PORT\n"
					  << "  Receiver mode: tv ADDRESS PORT\n";
			return 1;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}