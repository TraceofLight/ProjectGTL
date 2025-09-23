#include "pch.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Runtime/Core/Public/Class.h"
#include "Editor/Public/Camera.h"

IMPLEMENT_SINGLETON_CLASS_BASE(UConfigManager)

UConfigManager::UConfigManager()
	: EditorIniFileName("editor.ini")
{
	LoadEditorSetting();
}

UConfigManager::~UConfigManager()
{
	SaveEditorSetting();
}

void UConfigManager::SaveEditorSetting()
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

void UConfigManager::LoadEditorSetting()
{
	const FString& fileNameStr = EditorIniFileName.ToString();
	std::ifstream ifs(fileNameStr);
	if (!ifs.is_open())
	{
		CellSize = 1.0f;
		CameraSensitivity = UCamera::DEFAULT_SPEED;
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
