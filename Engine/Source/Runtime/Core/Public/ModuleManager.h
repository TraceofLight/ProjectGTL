#pragma once

#include "Runtime/Core/Public/Object.h"
#include <unordered_map>
#include <string>

/**
 * @brief 기본 모듈 인터페이스
 */
class IModuleInterface
{
public:
    virtual ~IModuleInterface() = default;
    
    virtual void StartupModule() {}
    virtual void ShutdownModule() {}
    virtual bool IsGameModule() const { return false; }
};

/**
 * @brief 언리얼 스타일의 모듈 관리자
 */
class FModuleManager
{
public:
    static FModuleManager& Get();
    
    template<typename T>
    T& LoadModuleChecked(const std::string& ModuleName)
    {
        IModuleInterface* Module = GetModulePtr(ModuleName);
        if (!Module)
        {
            Module = CreateModule<T>(ModuleName);
            if (Module)
            {
                Module->StartupModule();
                LoadedModules[ModuleName] = Module;
            }
        }
        
        return static_cast<T&>(*Module);
    }
    
    template<typename T>
    T* GetModulePtr(const std::string& ModuleName)
    {
        auto It = LoadedModules.find(ModuleName);
        if (It != LoadedModules.end())
        {
            return static_cast<T*>(It->second);
        }
        return nullptr;
    }
    
    void UnloadModule(const std::string& ModuleName);
    void ShutdownAllModules();

private:
    std::unordered_map<std::string, IModuleInterface*> LoadedModules;
    
    template<typename T>
    IModuleInterface* CreateModule(const std::string& ModuleName)
    {
        return new T();
    }
    
    IModuleInterface* GetModulePtr(const std::string& ModuleName);
};