#pragma once
#define CALLBACK __stdcall

#include <GLFW/glfw3.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <iostream>
#include <map>

#define MINIAUDIO_IMPLEMENTATION

#include "miniaudio.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <mutex>
#include <map>
#include <numbers>
#include <string>
#include <vector>
#include <conio.h>
#include <thread>
#include <chrono>

namespace audio
{
	class Device
	{
	public:
		using DataCallback = std::function<void(void*, const void*, ma_uint32)>;

		explicit Device(ma_device_config config)
		{
			config.pUserData = this;
			config.dataCallback = [](ma_device* device, void* output, const void* input, ma_uint32 frameCount)
			{
				static_cast<Device*>(device->pUserData)->OnDataCallback(output, input, frameCount);
			};

			if (ma_device_init(nullptr, &config, &m_device) != MA_SUCCESS)
			{
				throw std::runtime_error("Device initialization failed");
			}
		}

		void Start()
		{
			if (auto result = ma_device_start(&m_device); result != MA_SUCCESS)
			{
				throw std::runtime_error("Device start failed");
			}
		}

		void Stop()
		{
			ma_device_stop(&m_device);
		}

		ma_device* operator->() noexcept
		{
			return &m_device;
		}

		const ma_device* operator->() const noexcept
		{
			return &m_device;
		}

		void SetDataCallback(DataCallback dataCallback)
		{
			m_dataCallback = std::move(dataCallback);
		}

		~Device()
		{
			ma_device_uninit(&m_device);
		}

	private:
		void OnDataCallback(void* output, const void* input, ma_uint32 frameCount)
		{
			if (m_dataCallback)
				m_dataCallback(output, input, frameCount);
		}

		ma_device m_device{};
		DataCallback m_dataCallback;
	};

	class Player
	{
	public:
		using DataCallback = std::function<void(void* output, ma_uint32 frameCount)>;

		Player(ma_format format, ma_uint32 channels, ma_uint32 sampleRate = 48000)
			: m_device(CreateConfig(format, channels, sampleRate))
		{
			m_device.SetDataCallback([this](void* output, const void*, ma_uint32 frameCount)
			{
				if (m_dataCallback)
				{
					m_dataCallback(output, frameCount);
				}
			});
		}

		void Start()
		{
			m_device.Start();
		}

		void Stop()
		{
			m_device.Stop();
		}

		ma_format GetFormat() const noexcept
		{
			return m_device->playback.format;
		}

		ma_uint32 GetChannels() const noexcept
		{
			return m_device->playback.channels;
		}

		ma_uint32 GetSampleRate() const noexcept
		{
			return m_device->sampleRate;
		}

		void SetDataCallback(DataCallback callback)
		{
			m_dataCallback = std::move(callback);
		}

	private:
		static ma_device_config CreateConfig(ma_format format, ma_uint32 channels, ma_uint32 sampleRate)
		{
			auto config = ma_device_config_init(ma_device_type_playback);
			config.playback.format = format;
			config.playback.channels = channels;
			config.sampleRate = sampleRate;
			return config;
		}

		DataCallback m_dataCallback;
		Device m_device;
	};

	class ToneGenerator
	{
	public:
		ToneGenerator() = default;

		ToneGenerator(ma_uint32 sampleRate, ma_float frequency, ma_float amplitude = 0.3f)
			: m_sampleRate(sampleRate), m_frequency(frequency), m_amplitude(amplitude)
		{
			m_phaseShift = 2.0f * std::numbers::pi_v<float> * m_frequency / m_sampleRate;
		}

		ma_float GetNextSample()
		{
			if (!m_isActive) return 0.0f;

			float sample = m_amplitude * m_currentGain * std::sin(m_phase);
			m_phase = std::fmod(m_phase + m_phaseShift, 2.0f * std::numbers::pi_v<float>);

			if (m_isReleasing)
			{
				m_currentGain -= m_releaseStep;
				if (m_currentGain <= 0.0f)
				{
					m_currentGain = 0.0f;
					m_isActive = false;
				}
			}
			else
			{
				m_currentGain += m_attackStep;
				if (m_currentGain > 1)
				{
					m_currentGain = 1;
				}
			}

			return sample;
		}

		void NoteOn()
		{
			if (!m_isActive)
			{
				m_isActive = true;
				m_isReleasing = false;
				m_currentGain = 1.0f;
				m_phase = 0.0f;
				m_attackStep = 0;
			}
			else if (m_isReleasing)
			{
				m_isReleasing = false;
				m_attackStep = (1.0f - m_currentGain) / (0.01f * m_sampleRate);
			}
		}

		void NoteOff()
		{
			if (m_isActive && !m_isReleasing)
			{
				m_isReleasing = true;
				m_releaseStep = 1.0f / (0.5f * m_sampleRate);
			}
		}

		bool IsActive() const
		{
			return m_isActive;
		}

	private:
		ma_uint32 m_sampleRate;
		ma_float m_frequency;
		ma_float m_amplitude;
		ma_float m_phase = 0.0f;
		ma_float m_phaseShift;
		ma_float m_currentGain = 0.0f;
		bool m_isActive = false;
		bool m_isReleasing = false;
		ma_float m_releaseStep = 0.0f;

		ma_float m_attackStep = 0.0f;
	};
}

class VirtualPiano
{
public:
	VirtualPiano()
		: m_player(ma_format_f32, 1, 44100)
	{
		InitKeyboardMapping();

		for (const auto& [key, freq]: m_keyFrequencies)
		{
			m_toneGenerators.emplace(key, audio::ToneGenerator(m_player.GetSampleRate(), freq));
		}

		m_player.SetDataCallback([this](void* output, ma_uint32 frameCount)
		{
			auto samples = static_cast<float*>(output);

			for (ma_uint32 i = 0; i < frameCount; ++i)
			{
				samples[i] = 0.0f;
			}

			{
				std::lock_guard<std::mutex> lock(m_mutex);
				for (auto& [key, gen]: m_toneGenerators)
				{
					if (gen.IsActive())
					{
						for (ma_uint32 i = 0; i < frameCount; ++i)
						{
							samples[i] += gen.GetNextSample();
						}
					}
				}
			}

			for (ma_uint32 i = 0; i < frameCount; ++i)
			{
				samples[i] *= 0.3f;
			}

			{
				std::lock_guard<std::mutex> waveLock(m_waveMutex);
				m_lastWaveSamples.assign(samples, samples + frameCount);
			}
		});

		m_player.Start();
	}

	~VirtualPiano()
	{
		m_player.Stop();
	}

	void HandleKeyPress(char key)
	{
		if (m_keyFrequencies.count(key))
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_toneGenerators[key].NoteOn();
		}
	}

	void HandleKeyRelease(char key)
	{
		if (m_keyFrequencies.count(key))
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_toneGenerators[key].NoteOff();
		}
	}

	void PrintInstructions() const
	{
		std::cout << "Virtual Piano\n";
		std::cout << "Keyboard mapping:\n";
		for (const auto& [key, freq]: m_keyFrequencies)
		{
			std::cout << "'" << key << "' - " << freq << " Hz\n";
		}
		std::cout << "Press ESC to quit\n";
	}

	std::vector<float> GetWaveSamples() const
	{
		std::lock_guard<std::mutex> lock(m_waveMutex);
		return m_lastWaveSamples;
	}

private:
	mutable std::mutex m_mutex;

	void InitKeyboardMapping()
	{
		const std::vector<std::pair<char, float>> whiteKeys1 = {
			{ 'a', 261.63f },  // C4
			{ 's', 293.66f },  // D4
			{ 'd', 329.63f },  // E4
			{ 'f', 349.23f },  // F4
			{ 'g', 392.00f },  // G4
			{ 'h', 440.00f },  // A4
			{ 'j', 493.88f }   // B4
		};

		const std::vector<std::pair<char, float>> blackKeys1 = {
			{ 'w', 277.18f },  // C#4/Db4
			{ 'e', 311.13f },  // D#4/Eb4
			{ 't', 369.99f },  // F#4/Gb4
			{ 'y', 415.30f },  // G#4/Ab4
			{ 'u', 466.16f }   // A#4/Bb4
		};

		// Вторая октава (C5-C6)
		const std::vector<std::pair<char, float>> whiteKeys2 = {
			{ 'k',  523.25f },  // C5
			{ 'l',  587.33f },  // D5
			{ ';',  659.25f },  // E5
			{ '\'', 698.46f }, // F5
			{ 'z',  783.99f },  // G5
			{ 'x',  880.00f },  // A5
			{ 'c',  987.77f },  // B5
			{ 'v',  1046.50f }  // C6
		};

		const std::vector<std::pair<char, float>> blackKeys2 = {
			{ 'o',  554.37f },  // C#5/Db5
			{ 'p',  622.25f },  // D#5/Eb5
			{ '[',  739.99f },  // F#5/Gb5
			{ ']',  830.61f },  // G#5/Ab5
			{ '\\', 932.33f }  // A#5/Bb5
		};

		for (const auto& [key, freq]: whiteKeys1) m_keyFrequencies[key] = freq;
		for (const auto& [key, freq]: blackKeys1) m_keyFrequencies[key] = freq;
		for (const auto& [key, freq]: whiteKeys2) m_keyFrequencies[key] = freq;
		for (const auto& [key, freq]: blackKeys2) m_keyFrequencies[key] = freq;
	}

	audio::Player m_player;
	std::map<char, float> m_keyFrequencies;
	std::map<char, audio::ToneGenerator> m_toneGenerators;
	mutable std::mutex m_waveMutex;
	std::vector<float> m_lastWaveSamples;
};

class PianoView
{
public:
	PianoView(int width, int height)
		: m_windowWidth(width), m_windowHeight(height), m_window(nullptr),
		  m_piano(), m_whiteKeyWidth(width / 14.0f), m_blackKeyWidth(width / 28.0f)
	{
		m_keyMap = {
			// Первая октава
			{ GLFW_KEY_A,             'a' },
			{ GLFW_KEY_S,             's' },
			{ GLFW_KEY_D,             'd' },
			{ GLFW_KEY_F,             'f' },
			{ GLFW_KEY_G,             'g' },
			{ GLFW_KEY_H,             'h' },
			{ GLFW_KEY_J,             'j' },
			{ GLFW_KEY_W,             'w' },
			{ GLFW_KEY_E,             'e' },
			{ GLFW_KEY_T,             't' },
			{ GLFW_KEY_Y,             'y' },
			{ GLFW_KEY_U,             'u' },

			// Вторая октава
			{ GLFW_KEY_K,             'k' },
			{ GLFW_KEY_L,             'l' },
			{ GLFW_KEY_SEMICOLON,     ';' },
			{ GLFW_KEY_APOSTROPHE,    '\'' },
			{ GLFW_KEY_Z,             'z' },
			{ GLFW_KEY_X,             'x' },
			{ GLFW_KEY_C,             'c' },
			{ GLFW_KEY_V,             'v' },
			{ GLFW_KEY_O,             'o' },
			{ GLFW_KEY_P,             'p' },
			{ GLFW_KEY_LEFT_BRACKET,  '[' },
			{ GLFW_KEY_RIGHT_BRACKET, ']' },
			{ GLFW_KEY_BACKSLASH,     '\\' }
		};

		for (const auto& [key, note]: m_keyMap)
		{
			m_keyStates[note] = false;
		}
		//TODO: сделать клавиши управления по заданию

	}

	~PianoView()
	{
		if (m_window)
		{
			glfwDestroyWindow(m_window);
			glfwTerminate();
		}
	}

	void Run()
	{
		if (!glfwInit())
		{
			std::cerr << "Ошибка инициализации GLFW!" << std::endl;
			return;
		}

		m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "Virtual Piano", nullptr, nullptr);
		if (!m_window)
		{
			std::cerr << "Ошибка создания окна!" << std::endl;
			glfwTerminate();
			return;
		}

		glfwMakeContextCurrent(m_window);
		glfwSetWindowUserPointer(m_window, this);
		glfwSetKeyCallback(m_window, KeyCallback);
		glfwSetFramebufferSizeCallback(m_window, ResizeCallback);


		InitOpenGL();
		m_piano.PrintInstructions();

		while (!glfwWindowShouldClose(m_window))
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			auto newSamples = m_piano.GetWaveSamples();
			if (!newSamples.empty())
			{
				UpdateWaveform(newSamples);
			}

			DrawPiano();
			DrawWaveform();

			glfwSwapBuffers(m_window);
			glfwPollEvents();
		}
	}

private:
	int m_windowWidth;
	int m_windowHeight;
	GLFWwindow* m_window;
	VirtualPiano m_piano;
	float m_whiteKeyWidth;
	float m_blackKeyWidth;
	std::map<int, char> m_keyMap;
	std::map<char, bool> m_keyStates;
	std::vector<float> m_waveformHistory;
	const size_t WAVEFORM_HISTORY_SIZE = 2000;

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		auto* view = static_cast<PianoView*>(glfwGetWindowUserPointer(window));
		if (!view) return;

		if (view->m_keyMap.count(key))
		{
			char note = view->m_keyMap[key];
			if (action == GLFW_PRESS)
			{
				view->m_piano.HandleKeyPress(note);
				view->m_keyStates[note] = true;
			}
			else if (action == GLFW_RELEASE)
			{
				view->m_piano.HandleKeyRelease(note);
				view->m_keyStates[note] = false;
			}
		}
	}

	static void ResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto* view = static_cast<PianoView*>(glfwGetWindowUserPointer(window));
		if (view)
		{
			view->m_windowWidth = width;
			view->m_windowHeight = height;
			view->m_whiteKeyWidth = width / 14.0f;
			view->m_blackKeyWidth = width / 28.0f;

			glViewport(0, 0, width, height);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0, width, height, 0);
			glMatrixMode(GL_MODELVIEW);
		}
	}

	void InitOpenGL() const
	{
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0, m_windowWidth, m_windowHeight, 0);
		glMatrixMode(GL_MODELVIEW);

		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	}

	void DrawPiano()
	{
		const char whiteKeys[] = { 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', 'z', 'x', 'c', 'v' };
		for (int i = 0; i < 15; ++i)
		{
			char currentKey = whiteKeys[i];
			bool isPressed = m_keyStates.count(currentKey) && m_keyStates[currentKey];

			if (isPressed)
			{
				glColor3f(0.7f, 0.7f, 0.7f);
			}
			else
			{
				glColor3f(1.0f, 1.0f, 1.0f);
			}

			glBegin(GL_QUADS);
			glVertex2f(i * m_whiteKeyWidth, 0);
			glVertex2f((i + 1) * m_whiteKeyWidth, 0);
			glVertex2f((i + 1) * m_whiteKeyWidth, m_windowHeight);
			glVertex2f(i * m_whiteKeyWidth, m_windowHeight);
			glEnd();
		}

		const std::vector<std::pair<float, char>> blackKeys = {
			{ 1,  'w' },
			{ 2,  'e' },
			{ 4,  't' },
			{ 5,  'y' },
			{ 6,  'u' },

			{ 8,  'o' },
			{ 9,  'p' },
			{ 11, '[' },
			{ 12, ']' },
			{ 13, '\\' }
		};


		glColor3f(0.0f, 0.0f, 0.0f);
		for (const auto& [pos, key]: blackKeys)
		{
			float x = pos * m_whiteKeyWidth - m_blackKeyWidth / 2;

			if (m_keyStates.count(key) && m_keyStates[key])
			{
				glColor3f(0.4f, 0.4f, 0.4f);
			}
			else
			{
				glColor3f(0.0f, 0.0f, 0.0f);
			}

			glBegin(GL_QUADS);
			glVertex2f(x, 0);
			glVertex2f(x + m_blackKeyWidth, 0);
			glVertex2f(x + m_blackKeyWidth, m_windowHeight * 0.6f);
			glVertex2f(x, m_windowHeight * 0.6f);
			glEnd();
		}
	}

	void UpdateWaveform(const std::vector<float>& newSamples)
	{
		m_waveformHistory.insert(m_waveformHistory.end(), newSamples.begin(), newSamples.end());

		if (m_waveformHistory.size() > WAVEFORM_HISTORY_SIZE)
		{
			m_waveformHistory.erase(
				m_waveformHistory.begin(),
				m_waveformHistory.begin() + (m_waveformHistory.size() - WAVEFORM_HISTORY_SIZE)
			);
		}
	}

	void DrawWaveform()
	{
		if (m_waveformHistory.empty())
		{
			return;
		}

		glLineWidth(2.0f);
		glColor3f(0.0f, 1.0f, 0.0f);
		glBegin(GL_LINE_STRIP);

		float waveHeight = m_windowHeight * 0.5;
		float centerY = m_windowHeight * 0.9f;
		float step = static_cast<float>(m_windowWidth) / WAVEFORM_HISTORY_SIZE;

		for (size_t i = 0; i < m_waveformHistory.size(); ++i)
		{
			float x = i * step;
			float y = centerY - m_waveformHistory[i] * waveHeight;
			glVertex2f(x, y);
		}

		glEnd();

		glColor3f(0.5f, 0.5f, 0.5f);
		glBegin(GL_LINES);
		glVertex2f(0, centerY);
		glVertex2f(m_windowWidth, centerY);
		glEnd();
	}
};