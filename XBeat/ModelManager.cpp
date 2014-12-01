//===-- ModelManager.cpp - Defines the manager for PMX Model objects --*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------------===//
///
/// \file
/// \brief This file defines everything related to the PMX::Model manager, the
/// ModelManager class.
///
//===----------------------------------------------------------------------------===//

#include "ModelManager.h"

#include "PMX/PMXModel.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <stdexcept>

namespace fs = boost::filesystem;

ModelManager::ModelManager()
{
	ModelLoader.reset(new PMX::Loader);
}

ModelManager::~ModelManager()
{

}

void ModelManager::loadList()
{
	fs::path ModelPath(L"./Data/Models/"), CacheFilePath(L"./Data/ModelCache.dat");
	if (!fs::exists(ModelPath)) throw std::ios_base::failure("Model directory not found");

	if (loadFromCache(CacheFilePath, ModelPath))
		return;

	// Rebuild the cache file, since it is invalid
	loadInternal(ModelPath);
	saveToCache(CacheFilePath, ModelPath);
}

std::shared_ptr<PMX::Model> ModelManager::loadModel(const std::wstring &Name, std::shared_ptr<Physics::Environment> Physics)
{
	auto Path = KnownModels.find(Name);

	if (Path == KnownModels.end())
		return nullptr;

	std::shared_ptr<PMX::Model> Model(new PMX::Model);
	assert(Model);
	Model->SetPhysics(Physics);

	if (!Model->LoadModel(Path->second.wstring()))
		return nullptr;

	return Model;
}

bool ModelManager::loadFromCache(const boost::filesystem::path &FileName, const fs::path &ModelPath) {
	fs::path CacheFile(FileName);

	if (!fs::exists(CacheFile))
		return false;

	fs::ifstream inputStream;
	inputStream.open(CacheFile, std::ios::binary);

	if (!inputStream.good()) {
		inputStream.close();
		return false;
	}

	int64_t CacheTime;
	inputStream.read((char*)&CacheTime, sizeof(int64_t));

	// If the folder modify time is more recent than the stored in the cache, then we need to recache the model list
	if (fs::last_write_time(ModelPath) > CacheTime) {
		inputStream.close();
		return false;
	}

	uint64_t ModelCount;
	inputStream.read((char*)&ModelCount, sizeof(uint64_t));

	// This function is used to read a std::wstring from the cache file in its binary form
	auto readWideString = [](std::istream &inputStream) {
		uint64_t stringLength;
		inputStream.read((char*)&stringLength, sizeof(uint64_t));

		wchar_t *Buffer = new wchar_t[stringLength];
		inputStream.read((char*)Buffer, sizeof(wchar_t) * stringLength);

		std::wstring Output(Buffer, stringLength);

		delete[] Buffer;

		return Output;
	};

	// New operator :)
	while (ModelCount --> 0) {
		std::wstring Name = readWideString(inputStream);
		std::wstring Path = readWideString(inputStream);
		KnownModels[Name] = fs::path(Path);
	}

	inputStream.close();
	return true;
}

void ModelManager::loadInternal(const boost::filesystem::path &Path) {
	fs::directory_iterator EndIterator;

	for (fs::directory_iterator PathIterator(Path); PathIterator != EndIterator; ++PathIterator) {
		// If the current visited path is a directory, recusively look for a model
		if (fs::is_directory(PathIterator->status())) {
			loadInternal(PathIterator->path());
		}
		// Validates the PMX model extension
		else if (fs::is_regular_file(PathIterator->status()) && PathIterator->path().has_extension()) {
			if (boost::iequals(PathIterator->path().extension().generic_wstring(), L".pmx")) {
				try {
					auto desc = ModelLoader->getDescription(PathIterator->path().wstring());
					KnownModels[desc.name.japanese] = PathIterator->path();
				}
				catch (PMX::Loader::Exception &e) {
				}
			}
		}
	}
}

void ModelManager::saveToCache(const boost::filesystem::path &FileName, const fs::path &ModelPath) {
	fs::ofstream outputStream;
	outputStream.open(FileName, std::ios::binary);

	if (!outputStream.good()) {
		outputStream.close();
		return;
	}

	int64_t CacheTime = fs::last_write_time(ModelPath);
	outputStream.write((char*)&CacheTime, sizeof(int64_t));
	uint64_t ModelCount = KnownModels.size();
	outputStream.write((char*)&ModelCount, sizeof(uint64_t));

	// This function is used to write to the output stream a string in its binary form
	auto writeWideString = [](std::ostream &outputStream, const std::wstring &String) {
		uint64_t StringLength = (uint64_t)String.length();
		outputStream.write((char*)&StringLength, sizeof(uint64_t));
		outputStream.write((char*)String.c_str(), StringLength * sizeof(wchar_t));
	};

	for (auto &Model : KnownModels) {
		writeWideString(outputStream, Model.first);
		writeWideString(outputStream, Model.second.generic_wstring());
	}

	outputStream.close();
}
