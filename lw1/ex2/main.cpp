//
// Created by admin on 15.02.2025.
//

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;

struct Args
{
	std::string mode;
	int numProcesses = 0;
	std::string archiveName;
	std::string outputFolder;
};

void DecompressFile(const std::string& filename)
{
	std::string command = "gzip -d " + filename;
	if (system(command.c_str()))
	{
		throw std::runtime_error("Failed to decompress file: " + filename);
	}
}

void SequentialMode(const std::string& archiveName, const std::string& outputFolder)
{
	auto start = std::chrono::high_resolution_clock::now();

	std::string extractCommand = "tar -xf " + archiveName + " -C " + outputFolder;
	if (system(extractCommand.c_str()))
	{
		throw std::runtime_error("Failed to extract files from archive: " + archiveName);
	}

	std::vector<std::string> files;
	for (const auto& entry: fs::directory_iterator(outputFolder))
	{
		if (entry.path().extension() == ".gz")
		{
			files.push_back(entry.path().string());
		}
	}

	for (const auto& file: files)
	{
		DecompressFile(file);
	}

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	std::cout << "Total time: " << elapsed.count() << " seconds\n";
}

void ParallelMode(int numProcesses, const std::string& archiveName, const std::string& outputFolder)
{
	auto start = std::chrono::high_resolution_clock::now();

	std::string extractCommand = "tar -xf " + archiveName + " -C " + outputFolder;
	if (system(extractCommand.c_str()))
	{
		throw std::runtime_error("Failed to extract files from archive: " + archiveName);
	}

	std::vector<std::string> files;
	for (const auto& entry: fs::directory_iterator(outputFolder))
	{
		if (entry.path().extension() == ".gz")
		{
			files.push_back(entry.path().string());
		}
	}

	std::vector<pid_t> pids;
	for (const auto& file: files)
	{
		if (pids.size() >= numProcesses)
		{
			int status;
			pid_t finishedPid = waitpid(-1, &status, 0);
			auto it = std::find(pids.begin(), pids.end(), finishedPid);
			if (it != pids.end())
			{
				pids.erase(it);
			}
		}

		pid_t pid = fork();
		if (pid == 0)
		{
			DecompressFile(file);
			_exit(0);
		}
		else if (pid > 0)
		{
			pids.push_back(pid);
		}
		else
		{
			throw std::runtime_error("Failed to fork process");
		}
	}

	for (pid_t pid: pids)
	{
		waitpid(pid, nullptr, 0);
	}

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	std::cout << "Total time: " << elapsed.count() << " seconds\n";
}

Args ParseArgs(int argc, char* argv[])
{
	Args args;
	if (argc < 4)
	{
		throw std::invalid_argument(
			"Usage: " + std::string(argv[0]) + " -S|-P NUM-PROCESSES ARCHIVE-NAME OUTPUT-FOLDER");
	}

	args.mode = argv[1];
	if (args.mode == "-S")
	{
		args.archiveName = argv[2];
		args.outputFolder = argv[3];
	}
	else if (args.mode == "-P")
	{
		args.numProcesses = std::stoi(argv[2]);
		args.archiveName = argv[3];
		args.outputFolder = argv[4];
	}
	else
	{
		throw std::invalid_argument("Invalid mode. Use -S for sequential or -P for parallel mode.");
	}

	return args;
}

int main(int argc, char* argv[])
{
	try
	{
		Args args = ParseArgs(argc, argv);

		if (!fs::exists(args.outputFolder))
		{
			fs::create_directory(args.outputFolder);
		}

		if (args.mode == "-S")
		{
			SequentialMode(args.archiveName, args.outputFolder);
		}
		else if (args.mode == "-P")
		{
			ParallelMode(args.numProcesses, args.archiveName, args.outputFolder);
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}