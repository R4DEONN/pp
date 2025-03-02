#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <variant>
#include <stdexcept>
#include <fstream>
#include <thread>
#include <random>
#include <SFML/Graphics.hpp>

template<class... Ts>
struct overloads : Ts ...
{
	using Ts::operator()...;
};
template<class... Ts> overloads(Ts...) -> overloads<Ts...>;

using State = std::vector<std::vector<bool>>;

constexpr int CELL_SIZE = 8;
const sf::Color ALIVE_COLOR = sf::Color::Black;
const sf::Color DEAD_COLOR = sf::Color::White;

struct GenerateArgs
{
	std::string outFileName;
	int width{};
	int height{};
	float probability{};
};

struct StepArgs
{
	int numThread = 0;
	std::string outFileName;
	std::string inFileName;
};

struct VisualizeArgs
{
	int numThread = 0;
	std::string inFileName;
};

using VariantArgs = std::variant<GenerateArgs, StepArgs, VisualizeArgs>;

VariantArgs ParseArgs(int argc, char* argv[])
{
	if (argc < 2)
	{
		throw std::invalid_argument("Not enough arguments. Usage: \n"
									"life generate OUTPUT_FILE_NAME WIDTH HEIGHT PROBABILITY\n"
									"life step INPUT_FILE_NAME NUM_THREADS [OUTPUT_FILE_NAME]\n"
									"life visualize INPUT_FILE_NAME NUM_THREADS\n");
	}

	std::string mode = argv[1];

	if (mode == "generate")
	{
		if (argc != 6)
		{
			throw std::invalid_argument("Invalid arguments for 'generate'. Usage:\n"
										"life generate OUTPUT_FILE_NAME WIDTH HEIGHT PROBABILITY");
		}

		GenerateArgs args;
		args.outFileName = argv[2];
		args.width = std::stoi(argv[3]);
		args.height = std::stoi(argv[4]);
		args.probability = std::stof(argv[5]);
		return args;
	}
	else if (mode == "step")
	{
		if (argc < 4 || argc > 5)
		{
			throw std::invalid_argument("Invalid arguments for 'step'. Usage:\n"
										"life step INPUT_FILE_NAME NUM_THREADS [OUTPUT_FILE_NAME]");
		}

		StepArgs args;
		args.inFileName = argv[2];
		args.numThread = std::stoi(argv[3]);
		if (argc == 5)
		{
			args.outFileName = argv[4];
		}
		return args;
	}
	else if (mode == "visualize")
	{
		if (argc != 4)
		{
			throw std::invalid_argument("Invalid arguments for 'visualize'. Usage:\n"
										"life visualize INPUT_FILE_NAME NUM_THREADS");
		}

		VisualizeArgs args;
		args.inFileName = argv[2];
		args.numThread = std::stoi(argv[3]);
		return args;
	}
	else
	{
		throw std::invalid_argument("Unknown mode: " + mode);
	}
}

void GenerateField(GenerateArgs args)
{
	if (args.width <= 0 || args.height <= 0)
	{
		throw std::invalid_argument("Field dimensions must be positive integers");
	}
	if (args.probability < 0.0f || args.probability > 1.0f)
	{
		throw std::invalid_argument("Probability must be in range [0.0, 1.0]");
	}

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(0.0f, 1.0f);

	std::ofstream outFile(args.outFileName);
	if (!outFile.is_open())
	{
		throw std::runtime_error("Failed to open file: " + args.outFileName);
	}

	outFile << args.width << " " << args.height << "\n";

	for (int y = 0; y < args.height; ++y)
	{
		std::string row;
		row.reserve(args.width);

		for (int x = 0; x < args.width; ++x)
		{
			row += (dis(gen) < args.probability) ? '#' : ' ';
		}

		outFile << row << '\n';
	}
}

using namespace std::chrono;

struct Field
{
	int width;
	int height;
	State cells;
	State nextState;
};

Field ReadField(const std::string& filename)
{
	std::ifstream file(filename);
	if (!file)
	{
		throw std::runtime_error("Can't open file: " + filename);
	}

	int width, height;
	file >> width >> height;

	State cells(height, std::vector<bool>(width));
	std::string line;
	for (int y = 0; y < height && std::getline(file, line); ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			cells[y][x] = (line[x] == '#');
		}
	}
	return { width, height, cells, cells };
}

void WriteField(const std::string& filename, const Field& field)
{
	std::ofstream file(filename);
	if (!file)
	{
		throw std::runtime_error("Can't write to file: " + filename);
	}

	file << field.width << " " << field.height << "\n";
	for (const auto& row: field.cells)
	{
		for (bool cell: row)
		{
			file << (cell ? '#' : ' ');
		}
		file << "\n";
	}
}

void CalculateSection(int startY, int endY,
	const State& current,
	State& next,
	int width, int height)
{
	for (int y = startY; y < endY; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			int neighbors = 0;

			for (int dy = -1; dy <= 1; ++dy)
			{
				for (int dx = -1; dx <= 1; ++dx)
				{
					if (dx == 0 && dy == 0)
					{
						continue;
					}

					int nx = (x + dx + width) % width;
					int ny = (y + dy + height) % height;

					if (current[ny][nx])
					{
						neighbors++;
					}
				}
			}

			next[y][x] = (current[y][x])
						 ? (neighbors == 2 || neighbors == 3)
						 : (neighbors == 3);
		}
	}
}

void GenerateNextState(int width, int height, int numThreads, const State& currentState, State& nextState)
{
	std::vector<std::jthread> threads;
	const int rowsPerThread = height / numThreads;

	for (int i = 0; i < numThreads; ++i)
	{
		int start = i * rowsPerThread;
		int end = (i == numThreads - 1) ? height : start + rowsPerThread;
		threads.emplace_back(CalculateSection, start, end,
			cref(currentState), ref(nextState), width, height);
	}
}

void Step(const StepArgs& args)
{
	Field field = ReadField(args.inFileName);
	const int width = field.width;
	const int height = field.height;

	auto start = high_resolution_clock::now();

	GenerateNextState(width, height, args.numThread, field.cells, field.nextState);

	auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start);
	std::cout << duration.count() << "ms\n";

	field.cells.swap(field.nextState);
	WriteField(args.outFileName.empty() ? args.inFileName : args.outFileName, field);
}

void UpdatePixels(int width, int height, State& state, std::vector<sf::Uint8>& pixels)
{
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const int idx = (y * width + x) * 4;
			const sf::Color color = state[y][x] ? ALIVE_COLOR : DEAD_COLOR;
			pixels[idx] = color.r;
			pixels[idx + 1] = color.g;
			pixels[idx + 2] = color.b;
		}
	}
}

void UpdateState(bool paused, sf::Texture& texture, Field& field, int numThread, std::vector<long>& stepTimes, std::vector<sf::Uint8>& pixels )
{
	if (!paused)
	{
		//TODO: Переписать
		auto stepStart = high_resolution_clock::now();

		GenerateNextState(field.width, field.height, numThread, field.cells, field.nextState);
		field.cells.swap(field.nextState);

		auto stepDuration = duration_cast<microseconds>(
			high_resolution_clock::now() - stepStart
		);
		stepTimes.push_back(stepDuration.count());
	}

	UpdatePixels(field.width, field.height, field.cells, pixels);
	texture.update(pixels.data());
}

void Visualize(VisualizeArgs args)
{
	Field field = ReadField(args.inFileName);
	const int width = field.width;
	const int height = field.height;

	sf::RenderWindow window(
		sf::VideoMode(width * CELL_SIZE, height * CELL_SIZE),
		"Game of Life"
	);
	window.setFramerateLimit(60);

	sf::Texture texture;
	texture.create(width, height);
	sf::Sprite sprite(texture);
	sprite.setScale(CELL_SIZE, CELL_SIZE);

	std::vector<long> stepTimes;
	auto lastTitleUpdate = high_resolution_clock::now();
	bool paused = false;

	std::vector<sf::Uint8> pixels(width * height * 4, 255);

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				window.close();
			}
			if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space)
			{
				paused = !paused;
			}
		}

		UpdateState(paused, texture, field, args.numThread, stepTimes, pixels);

		auto now = high_resolution_clock::now();
		if (now - lastTitleUpdate >= 1s)
		{
			if (!stepTimes.empty())
			{
				long long total = 0;
				for (auto t : stepTimes) total += t;
				double avg = total / stepTimes.size() / 1000.0;

				window.setTitle("Game of Life - Avg step: "
								+ std::to_string(avg)
								+ "ms (Space to pause)");
				stepTimes.clear();
			}
			lastTitleUpdate = now;
		}

		window.clear(DEAD_COLOR);
		window.draw(sprite);
		window.display();
	}
}

int main(int argc, char* argv[])
{
	try
	{
		auto args = ParseArgs(argc, argv);

		std::visit(overloads
			{
				[](const GenerateArgs& args)
				{ GenerateField(args); },
				[](const StepArgs& args)
				{ Step(args); },
				[](const VisualizeArgs& args)
				{ Visualize(args); }
			}, args);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}