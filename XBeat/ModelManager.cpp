#include "ModelManager.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>
#include <stdexcept>
#include "PMX/PMXModel.h"

namespace fs = boost::filesystem;

ModelManager::ModelManager()
{
}


ModelManager::~ModelManager()
{
}

void ModelManager::LoadList()
{
	fs::path modelPath(L"./Data/Models/");
	if (!fs::exists(modelPath)) throw std::ios_base::failure("Model directory not found");

	// Check for cache file
	fs::path cacheFile(L"./Data/ModelCache.dat");

	bool reloadCache = true;

	if (fs::exists(cacheFile)) {
		fs::ifstream ifs;
		ifs.open(cacheFile, std::ios::binary);
		if (ifs.good()) {
			uint64_t cacheTime;
			ifs.read((char*)&cacheTime, sizeof(uint64_t));

			if (fs::last_write_time(modelPath) > cacheTime)
				reloadCache = true;
			else {
				uint64_t count;
				ifs.read((char*)&count, sizeof(uint64_t));

				auto readwstr = [](std::istream &is) {
					uint64_t len;
					
					is.read((char*)&len, sizeof(uint64_t));
					wchar_t *buf = new wchar_t[len];
					is.read((char*)buf, sizeof(wchar_t) * len);
					std::wstring out(buf, len);
					delete[] buf;

					return out;
				};

				while (count --> 0) {
					std::wstring name = readwstr(ifs);
					std::wstring path = readwstr(ifs);
					m_models[name] = fs::path(path);
				}
			}

			ifs.close();

			if (!reloadCache) return;
		}
	}

	if (reloadCache) loadInternal(modelPath);

	// Rebuild the cache file
	fs::ofstream ofs;
	ofs.open(cacheFile, std::ios::binary);
	if (ofs.good()) {
		uint64_t time = fs::last_write_time(modelPath);
		ofs.write((char*)&time, sizeof(uint64_t));
		uint64_t count = m_models.size();
		ofs.write((char*)&count, sizeof(uint64_t));

		auto writestr = [](std::ofstream &os, const std::wstring &str) {
			uint64_t len = (uint64_t)str.length();
			os.write((char*)&len, sizeof(uint64_t));
			os.write((char*)str.c_str(), len * sizeof(wchar_t));
		};

		for (auto i : m_models) {
			writestr(ofs, i.first);
			writestr(ofs, i.second.generic_wstring());
		}

		ofs.close();
	}
}

std::shared_ptr<PMX::Model> ModelManager::LoadModel(const std::wstring &name)
{
	auto path = m_models.find(name);

	if (path == m_models.end()) return nullptr;

	std::shared_ptr<PMX::Model> model(new PMX::Model);

	if (!model) return nullptr;

	if (!model->LoadModel(path->second.wstring())) return nullptr;

	return model;
}

void ModelManager::loadInternal(const boost::filesystem::path &p) {
	fs::directory_iterator end;

	for (fs::directory_iterator i(p); i != end; ++i) {
		if (fs::is_directory(i->status())) {
			loadInternal(i->path());
		}
		else if (fs::is_regular_file(i->status()) && i->path().has_extension()) {
			if (boost::iequals(i->path().extension().generic_wstring(), L".pmx")) {
				auto desc = loader.GetDescription(i->path().wstring());
				m_models[desc.name.japanese] = i->path();
			}
		}
	}
}
