#include "pch.h"
#include "Global/Paths.h"

path InitProjectRootDir()
{
    WCHAR Buffer[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, Buffer, MAX_PATH);
    path ExecutionFilePath = Buffer;
    return ExecutionFilePath.parent_path();
}

path FPaths::ProjectRootDir = InitProjectRootDir();

const path& FPaths::GetProjectRootDir()
{
	return ProjectRootDir;
}

path FPaths::GetShaderPath()
{
	return GetProjectRootDir()/ "Asset" / "Shader" / "";
}

path FPaths::GetContentPath()
{
	return GetProjectRootDir()/ "Asset" / "";
}

path FPaths::ConvertToProjectAbsolutePath(const path& InPath)
{
	return GetProjectRootDir() / InPath;
}
