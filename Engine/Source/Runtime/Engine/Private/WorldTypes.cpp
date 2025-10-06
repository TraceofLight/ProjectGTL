#include "pch.h"
#include "Runtime/Engine/Public/WorldTypes.h"
#include "Runtime/Engine/Public/World.h"

bool FWorldContext::IsEditorWorld() const
{
	return ThisCurrentWorld && ThisCurrentWorld->GetWorldType() == EWorldType::Editor;
}

bool FWorldContext::IsPIEWorld() const
{
	return ThisCurrentWorld && ThisCurrentWorld->GetWorldType() == EWorldType::PIE;
}

bool FWorldContext::IsGameWorld() const
{
	return ThisCurrentWorld && ThisCurrentWorld->GetWorldType() == EWorldType::Game;
}
