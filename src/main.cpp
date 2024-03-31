#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <vector>

#include "filesystem/fs_util.hpp"
#include "parsing/fileReader.hpp"

int main() {
	const auto cpRootSavePath = files::getCpSaveFolder();
	const auto pointOfNoReturnSavePath = files::findLastPointOfNoReturnSave(cpRootSavePath);

	std::wcout << pointOfNoReturnSavePath.native() << std::endl;

	const auto savePath = pointOfNoReturnSavePath / L"sav.dat";

	std::wcout << savePath.native() << std::endl;
	std::println("Size: {}", std::filesystem::file_size(savePath));
	const auto bufferSize = std::filesystem::file_size(savePath);
	auto fileBuffer = std::vector<std::byte>(bufferSize);

	{
		auto fileStream = std::ifstream{ savePath, std::ios_base::binary };
		fileStream.read(reinterpret_cast<char*>(fileBuffer.data()), bufferSize);
	}

	parser::beginParse(fileBuffer);
}