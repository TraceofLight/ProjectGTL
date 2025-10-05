#include "pch.h"
#include "Runtime/Subsystem/Config/Public/ConfigSubsystem.h"

#include "Runtime/Actor/Public/CameraActor.h"
#include "Runtime/Core/Public/Class.h"

IMPLEMENT_CLASS(UConfigSubsystem, UEngineSubsystem)

UConfigSubsystem::UConfigSubsystem()
	: EditorIniFileName("editor.ini")
{
	// 생성자에서는 초기화하지 않음 - Initialize()에서 처리
}

UConfigSubsystem::~UConfigSubsystem()
{
	// 소멸자에서는 정리하지 않음 - Deinitialize()에서 처리
}

void UConfigSubsystem::Initialize()
{
	Super::Initialize();
	LoadEditorSetting();
	UE_LOG("ConfigSubsystem: Initialized");
}

void UConfigSubsystem::Deinitialize()
{
	SaveEditorSetting();
	Super::Deinitialize();
	UE_LOG("ConfigSubsystem: Deinitialized");
}

void UConfigSubsystem::SaveEditorSetting()
{
	std::ofstream ofs(EditorIniFileName.ToString());
	if (ofs.is_open())
	{
		ofs << "CellSize=" << CellSize << "\n";
		ofs << "CameraSensitivity=" << CameraSensitivity << "\n";
		ofs << "SplitV=" << SplitV << "\n";
		ofs << "SplitH=" << SplitH << "\n";
	}
}

void UConfigSubsystem::LoadEditorSetting()
{
	const FString& fileNameStr = EditorIniFileName.ToString();
	std::ifstream ifs(fileNameStr);
	if (!ifs.is_open())
	{
		CellSize = 1.0f;
		CameraSensitivity = ACameraActor::DEFAULT_SPEED;
		SplitV = 0.5f;   // ▼ 기본값
		SplitH = 0.5f;   // ▼ 기본값
		return;
	}

	std::string line;
	while (std::getline(ifs, line))
	{
		if (line.rfind("CellSize=", 0) == 0)
			CellSize = std::stof(line.substr(9));
		else if (line.rfind("CameraSensitivity=", 0) == 0)
			CameraSensitivity = std::stof(line.substr(18));
		else if (line.rfind("SplitV=", 0) == 0)
			SplitV = std::stof(line.substr(7));
		else if (line.rfind("SplitH=", 0) == 0)
			SplitH = std::stof(line.substr(7));
	}
}
