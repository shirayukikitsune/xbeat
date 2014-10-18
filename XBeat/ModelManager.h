//===-- ModelManager.h - Declares the manager for PMX Model objects --*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares everything related to the PMX::Model manager, the
/// ModelManager class.
///
//===---------------------------------------------------------------------------===//

#pragma once

#include <boost/filesystem/path.hpp>
#include <map>
#include <memory>
#include <string>

namespace PMX { class Loader; class Model; }

/// \brief Class that manages all PMX::Model in the Data folder
class ModelManager
{
public:
	typedef std::map<std::wstring, boost::filesystem::path> ModelList;

	ModelManager();
	~ModelManager();

	/// \brief Populates the list of available models
	void loadList();

	/// \brief Loads a particular model from the list, by its name
	///
	/// \param [in] name The name of the model to be loaded
	std::shared_ptr<PMX::Model> loadModel(const std::wstring &name);

	/// \brief Returns a copy of the KnownModels
	ModelList getKnownModels() const { return KnownModels; }

private:
	ModelList KnownModels;
	std::unique_ptr<PMX::Loader> ModelLoader;

	/// \brief Loads the model list from the cache file
	///
	/// \param [in] FileName The path to the cache file
	/// \param [in] ModelPath The path that the models are stored in
	///
	/// \returns false if the cache file failed to be loaded or if the cache is invalid
	bool loadFromCache(const boost::filesystem::path &FileName, const boost::filesystem::path &ModelPath);

	/// \brief Does the dirty job of populating the KnownModels if the cache needs to be revalidated
	void loadInternal(const boost::filesystem::path &Path);

	/// \brief Saves the cache to the cache file
	///
	/// \param [in] FileName The path to the cache file
	/// \param [in] ModelPath The path that the models are stored in
	void saveToCache(const boost::filesystem::path &FileName, const boost::filesystem::path &ModelPath);
};

