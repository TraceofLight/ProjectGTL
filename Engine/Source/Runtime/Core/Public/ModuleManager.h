#pragma once

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
 * @brief 모듈 관리자
 */
class FModuleManager
{
public:
    static FModuleManager& GetInstance();

    template<typename T>
    T& LoadModuleChecked(const FString& ModuleName)
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
    T* GetModulePtr(const FString& ModuleName)
    {
	    IModuleInterface** ModulePtr = LoadedModules.Find(ModuleName);

    	if (ModulePtr != nullptr)
    	{
    		return static_cast<T*>(*ModulePtr);
    	}

    	return nullptr;
    }

    void UnloadModule(const FString& ModuleName);
    void ShutdownAllModules();

private:
    TMap<FString, IModuleInterface*> LoadedModules;

    template<typename T>
    IModuleInterface* CreateModule(const FString& ModuleName)
    {
        return new T();
    }

    IModuleInterface* GetModulePtr(const FString& ModuleName);
};
