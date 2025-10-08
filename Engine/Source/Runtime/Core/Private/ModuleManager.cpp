#include "pch.h"
#include "Runtime/Core/Public/ModuleManager.h"

FModuleManager& FModuleManager::GetInstance()
{
    static FModuleManager Instance;
    return Instance;
}

IModuleInterface* FModuleManager::GetModulePtr(const FString& ModuleName)
{
	IModuleInterface** ModulePtr = LoadedModules.Find(ModuleName);

	if (ModulePtr != nullptr)
	{
		return *ModulePtr;
	}

	return nullptr;
}

void FModuleManager::UnloadModule(const FString& ModuleName)
{
	IModuleInterface** ModulePtr = LoadedModules.Find(ModuleName);

	if (ModulePtr != nullptr)
	{
		IModuleInterface* Module = *ModulePtr;

		Module->ShutdownModule();
		delete Module;

		LoadedModules.Remove(ModuleName);
	}
}

void FModuleManager::ShutdownAllModules()
{
    for (auto& Pair : LoadedModules)
    {
        Pair.second->ShutdownModule();
        delete Pair.second;
    }
    LoadedModules.Empty();
}
