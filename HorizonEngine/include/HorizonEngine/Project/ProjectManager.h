#pragma once

#ifndef HZN_PROJECT_MANAGER_H
#define HZN_PROJECT_MANAGER_H

#include <memory>
#include <string>
#include <filesystem>
#include <vector>

#include "HorizonEngine/Project/Project.h"

namespace Hzn
{
	class Project;
	class SceneManager;
	class AssetManager;

	class ProjectManager
	{
		//! Gives Scene Manager acces to Project Manager.
		friend class SceneManager;
		//! Gives Asset Manager acces to Project Manager.
		friend class AssetManager;
	public:
		/**
		 * \
		 */
		
		/// <summary>
		/// brief Creates new, or opens a project depending on the path provided.
		///	If you provide a directory path, it generates a new project in that directory.
		///	If you provide a file path, it checks whether this path is to the file.hzn,
		///	and depending on that file, the project is loaded.
		///	param name name of the folder to create project.
		///	param directoryPath directory in which the project directory should be created.
		///	return
		/// </summary>
		/// <param name="name"></param>
		/// <param name="directoryPath"></param>
		/// <returns></returns>
		static std::shared_ptr<Project> create(const std::string& name, const std::filesystem::path& directoryPath);
		static std::shared_ptr<Project> open(const std::filesystem::path& projectFilePath);
		static bool isOpen() { return s_Project != nullptr; }
		//! returns active project
		static std::shared_ptr<Project> getActiveProject() { return s_Project; }
		static std::vector<std::filesystem::path> getAllScenes();

		/// <summary>
		/// File handling functionality
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		static bool newScene(const std::string& name);
		static bool openScene(const std::filesystem::path& sceneFilePath);
		static bool closeScene();
		static bool saveScene();
		static bool save();
		static bool close();

	private:
		//!	Current project variable
		static std::shared_ptr<Project> s_Project;
	};
}

#endif