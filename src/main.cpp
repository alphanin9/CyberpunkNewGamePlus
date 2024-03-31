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
	const auto metadataPath = pointOfNoReturnSavePath / L"metadata.9.json";

	std::wcout << savePath.native() << std::endl;
	std::println("Size: {}", std::filesystem::file_size(savePath));

	parser::Parser fileParser;

	const auto ret = fileParser.parseSavegame(savePath);
	const auto ret2 = fileParser.parseMetadata(metadataPath);
}