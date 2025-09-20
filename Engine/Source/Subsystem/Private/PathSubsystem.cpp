#include "pch.h"
#include "Subsystem/Public/PathSubsystem.h"

IMPLEMENT_CLASS(UPathSubsystem, UEngineSubsystem)

void UPathSubsystem::Initialize()
{
	Super::Initialize();

	InitializeRootPath();
	GetEssentialPath();
	ValidateAndCreateDirectories();

	UE_LOG("PathSubsystem: Initialized Successfully");
	UE_LOG("PathSubsystem: Solution Path: %s", RootPath.string().c_str());
	UE_LOG("PathSubsystem: Asset Path: %s", AssetPath.string().c_str());
}

/**
 * @brief 프로그램 실행 파일을 기반으로 Root Path 세팅하는 함수
 */
void UPathSubsystem::InitializeRootPath()
{
	// Get Execution File Path
	wchar_t ProgramPath[MAX_PATH];
	GetModuleFileNameW(nullptr, ProgramPath, MAX_PATH);

	// Add Root Path
	RootPath = path(ProgramPath).parent_path();
}

/**
 * @brief 프로그램 실행에 필요한 필수 경로를 추가하는 함수
 */
void UPathSubsystem::GetEssentialPath()
{
	// Add Essential
	AssetPath = RootPath / L"Asset";
	ShaderPath = AssetPath / L"Shader";
	TexturePath = AssetPath / "Texture";
	ModelPath = AssetPath / "Model";
	AudioPath = AssetPath / "Audio";
	WorldPath = AssetPath / "World";
	ConfigPath = AssetPath / "Config";
	FontPath = AssetPath / "Font";
}

/**
 * @brief 경로 유효성을 확인하고 없는 디렉토리를 생성하는 함수
 */
void UPathSubsystem::ValidateAndCreateDirectories() const
{
	TArray<path> DirectoriesToCreate = {
		AssetPath,
		ShaderPath,
		TexturePath,
		ModelPath,
		AudioPath,
		WorldPath,
		ConfigPath,
		FontPath
	};

	for (const auto& Directory : DirectoriesToCreate)
	{
		try
		{
			if (!exists(Directory))
			{
				create_directories(Directory);
				UE_LOG("PathSubsystem: Created Directory: %s", Directory.string().c_str());
			}
			else
			{
				UE_LOG("PathSubsystem: Directory Exists: %s", Directory.string().c_str());
			}
		}
		catch (const filesystem::filesystem_error& e)
		{
			UE_LOG("PathSubsystem: Failed To Create Directory %s: %s", Directory.string().c_str(), e.what());
			assert(!"Asset 경로 생성 에러 발생");
		}
	}
}