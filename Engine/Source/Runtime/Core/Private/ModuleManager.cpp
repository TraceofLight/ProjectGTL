#include "pch.h"
#include "Runtime/Core/Public/ModuleManager.h"

FModuleManager& FModuleManager::Get()
{
    static FModuleManager Instance;
    return Instance;
}

IModuleInterface* FModuleManager::GetModulePtr(const std::string& ModuleName)
{
    auto It = LoadedModules.find(ModuleName);
    if (It != LoadedModules.end())
    {
        return It->second;
    }
    return nullptr;
}

void FModuleManager::UnloadModule(const std::string& ModuleName)
{
    auto It = LoadedModules.find(ModuleName);
    if (It != LoadedModules.end())
    {
        It->second->ShutdownModule();
        delete It->second;
        LoadedModules.erase(It);
    }
}

void FModuleManager::ShutdownAllModules()
{
    for (auto& Pair : LoadedModules)
    {
        Pair.second->ShutdownModule();
        delete Pair.second;
    }
    LoadedModules.clear();
}