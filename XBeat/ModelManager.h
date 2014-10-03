#pragma once

#include <memory>
#include <string>
#include <map>
#include <boost/filesystem/path.hpp>

#include "PMX/PMXLoader.h"

namespace Renderer { class Model; }

class ModelManager
{
public:
	ModelManager();
	~ModelManager();

	void LoadList();

	std::shared_ptr<PMX::Model> LoadModel(const std::wstring &name);

private:
	std::map<std::wstring, boost::filesystem::path> m_models;

	void loadInternal(const boost::filesystem::path &p);

	PMX::Loader loader;
};

