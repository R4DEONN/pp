#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <stdexcept>

struct Args
{
	std::string mode;
	int numProcesses = 0;
	std::string archiveName;
	std::vector<std::string> files;
};

void CompressFile(const std::string& filename)
{
	std::string command = "gzip -c " + filename + " > " + filename + ".gz";
	if (system(command.c_str()))
	{
		throw std::runtime_error("Failed to compress file: " + filename);
	}
}

void CreateArchive(const std::string& archiveName, const std::vector<std::string>& files)
{
	std::string command = "tar -cf " + archiveName + " ";
	for (const auto& file: files)
	{
		command += file + ".gz ";
	}
	if (system(command.c_str()))
	{
		throw std::runtime_error("Failed to create archive: " + archiveName);
	}
}

void SequentialMode(const std::string& archiveName, const std::vector<std::string>& files)
{
	auto start = std::chrono::high_resolution_clock::now();

	for (const auto& file: files)
	{
		CompressFile(file);
	}

	CreateArchive(archiveName, files);

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	std::cout << "Total time: " << elapsed.count() << " seconds\n";
}

void ParallelMode(int numProcesses, const std::string& archiveName, const std::vector<std::string>& files)
{
	auto start = std::chrono::high_resolution_clock::now();

	std::vector<pid_t> pids;
	//TODO: разбить на функции
	for (const auto& file: files)
	{
		if (pids.size() >= numProcesses)
		{
			int status;
			pid_t finishedPid = waitpid(-1, &status, 0);
			//TODO: std::erase
			auto it = std::find(pids.begin(), pids.end(), finishedPid);
			if (it != pids.end())
			{
				pids.erase(it);
			}
		}

		pid_t pid = fork();
		if (pid == 0)
		{
			CompressFile(file);
			_exit(0);//TODO: return bool
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

	auto archiveStart = std::chrono::high_resolution_clock::now();
	CreateArchive(archiveName, files);
	auto end = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> totalElapsed = end - start;
	std::chrono::duration<double> archiveElapsed = end - archiveStart;

	std::cout << "Total time: " << totalElapsed.count() << " seconds\n";
	std::cout << "Archive creation time: " << archiveElapsed.count() << " seconds\n";
}

Args ParseArgs(int argc, char* argv[])
{
	Args args;

	if (argc < 4)
	{
		throw std::invalid_argument(
			"Usage: " + std::string(argv[0]) + " -S|-P NUM-PROCESSES ARCHIVE-NAME [INPUT-FILES]");
	}

	args.mode = argv[1];
	if (args.mode == "-S")
	{
		args.archiveName = argv[2];
		for (int i = 3; i < argc; ++i)
		{
			args.files.emplace_back(argv[i]);
		}
	}
	else if (args.mode == "-P")
	{
		args.numProcesses = std::stoi(argv[2]);
		args.archiveName = argv[3];
		for (int i = 4; i < argc; ++i)
		{
			args.files.emplace_back(argv[i]);
		}
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

		if (args.mode == "-S")
		{
			SequentialMode(args.archiveName, args.files);
		}
		else if (args.mode == "-P")
		{
			ParallelMode(args.numProcesses, args.archiveName, args.files);
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}