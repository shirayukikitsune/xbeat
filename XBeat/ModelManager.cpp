#include "ModelManager.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <stdexcept>
#include "PMX/PMXModel.h"

ModelManager::ModelManager()
{
}


ModelManager::~ModelManager()
{
}

void ModelManager::LoadList()
{
	boost::filesystem::path modelPath(L"./Data/Models/");
	if (!exists(modelPath)) throw std::ios_base::failure("Model directory not found");

	loadInternal(modelPath);
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
	using namespace boost::filesystem;
	directory_iterator end;

	for (directory_iterator i(p); i != end; ++i) {
		if (is_directory(i->status())) {
			loadInternal(i->path());
		}
		else if (is_regular_file(i->status()) && i->path().has_extension()) {
			if (boost::iequals(i->path().extension().generic_wstring(), L".pmx")) {
				auto desc = loader.GetDescription(i->path().wstring());
				m_models[desc.name.japanese] = i->path();
			}
		}
	}
}
