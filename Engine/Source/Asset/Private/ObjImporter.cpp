#include "pch.h"
#include "Asset/Public/ObjImporter.h"
#include <algorithm>
#include <utility>

bool FObjImporter::ImportObjFile(const FString& InFilePath, TArray<FObjInfo>& OutObjectInfos)
{
	std::ifstream File(InFilePath.c_str());
	if (!File.is_open())
	{
		return false;
	}

	OutObjectInfos.clear();

	// 전역 데이터 배열 (OBJ 파일은 전역 정점/UV/노멀 목록을 가짐)
	TArray<FVector> GlobalVertices;
	TArray<FVector2> GlobalUVs;
	TArray<FVector> GlobalNormals;

	FString ObjDirectory;
	size_t LastSlash = InFilePath.find_last_of("/\\");
	if (LastSlash != FString::npos)
	{
		ObjDirectory = InFilePath.substr(0, LastSlash + 1); // 마지막 역슬래시까지 포함
	}

	TMap<FString, FObjMaterialInfo> MaterialLibrary;

	FObjInfo* CurrentObject = nullptr;
	int32 CurrentSectionIndex = -1;
	FString CurrentGroupName;
	FString CurrentMaterialName;

	std::string Line;
	while (std::getline(File, Line))
	{
		FString ObjLine(Line.c_str());

		// 첫 번째 객체가 없으면 임시 객체 생성
		if (CurrentObject == nullptr)
		{
			OutObjectInfos.emplace_back("TempObject");
			CurrentObject = &OutObjectInfos.back();
			CurrentGroupName = CurrentObject->ObjectName;
			CurrentSectionIndex = -1;
			CurrentMaterialName.clear();
		}

		ParseOBJLine(ObjLine,
			*CurrentObject,
			OutObjectInfos,
			GlobalVertices,
			GlobalUVs,
			GlobalNormals,
			CurrentSectionIndex,
			CurrentGroupName,
			CurrentMaterialName,
			MaterialLibrary,
			ObjDirectory);

		// 새 객체가 추가되었으면 CurrentObject 업데이트
		if (!OutObjectInfos.empty())
		{
			CurrentObject = &OutObjectInfos.back();
		}
	}

	File.close();

	// 쉬운 처리를 위해 각 객체에 전역 데이터 복사
	for (FObjInfo& ObjectInfo : OutObjectInfos)
	{
		ObjectInfo.VertexList = GlobalVertices;
		ObjectInfo.UVList = GlobalUVs;
		ObjectInfo.NormalList = GlobalNormals;
	}

	return true;
}

bool FObjImporter::ParseMaterialLibrary(const FString& InMTLFilePath, TMap<FString, FObjMaterialInfo>& OutMaterialLibrary)
{
	std::ifstream File(InMTLFilePath.c_str());
	if (!File.is_open())
	{
		return false;
	}
	UE_LOG_WARNING("Loading MTL file: %s", InMTLFilePath.c_str());
	FObjMaterialInfo* CurrentMaterial = nullptr;

	std::string Line;
	while (std::getline(File, Line))
	{
		FString MTLLine = TrimString(FString(Line.c_str()));

		if (MTLLine.empty() || MTLLine[0] == '#')
		{
			continue; // 빈 줄과 주석은 건너뜀
		}

		TArray<FString> Tokens;
		SplitString(MTLLine, ' ', Tokens);

		if (Tokens.empty())
		{
			continue;
		}

		if (Tokens[0] == "newmtl" && Tokens.size() > 1)
		{
			// 새 재질
			OutMaterialLibrary[Tokens[1]] = FObjMaterialInfo(Tokens[1]);
			CurrentMaterial = &OutMaterialLibrary[Tokens[1]];
		}
		else if (CurrentMaterial != nullptr)
		{
			if (Tokens[0] == "map_Kd" && Tokens.size() > 1)
			{
				CurrentMaterial->DiffuseTexturePath = Tokens[1];
			}
			else if (Tokens[0] == "map_Kn" && Tokens.size() > 1)
			{
				CurrentMaterial->NormalTexturePath = Tokens[1];
			}
			else if (Tokens[0] == "map_Ks" && Tokens.size() > 1)
			{
				CurrentMaterial->SpecularTexturePath = Tokens[1];
			}
			else if (Tokens[0] == "Ka" && Tokens.size() > 3)
			{
				CurrentMaterial->AmbientColorScalar = FVector(
					std::stof(Tokens[1].c_str()),
					std::stof(Tokens[2].c_str()),
					std::stof(Tokens[3].c_str())
				);
			}
			else if (Tokens[0] == "Kd" && Tokens.size() > 3)
			{
				CurrentMaterial->DiffuseColorScalar = FVector(
					std::stof(Tokens[1].c_str()),
					std::stof(Tokens[2].c_str()),
					std::stof(Tokens[3].c_str())
				);
			}
			else if (Tokens[0] == "Ks" && Tokens.size() > 3)
			{
				CurrentMaterial->SpecularColorScalar = FVector(
					std::stof(Tokens[1].c_str()),
					std::stof(Tokens[2].c_str()),
					std::stof(Tokens[3].c_str())
				);
			}
			else if (Tokens[0] == "Ns" && Tokens.size() > 1)
			{
				CurrentMaterial->ShininessScalar = std::stof(Tokens[1].c_str());
			}
			else if (Tokens[0] == "d" && Tokens.size() > 1)
			{
				CurrentMaterial->TransparencyScalar = std::stof(Tokens[1].c_str());
			}
		}
	}

	File.close();
	return true;
}

bool FObjImporter::ConvertToStaticMesh(const TArray<FObjInfo>& InObjectInfos, FStaticMesh& OutStaticMesh)
{
	if (InObjectInfos.empty())
	{
		return false;
	}

	OutStaticMesh.Vertices.clear();
	OutStaticMesh.Indices.clear();
	OutStaticMesh.Sections.clear();

	// 모든 객체를 하나의 스태틱 메시로 결합
	for (const FObjInfo& ObjectInfo : InObjectInfos)
	{
		TArray<FVertex> ObjectVertices;
		TArray<uint32> ObjectIndices;

		ConvertToTriangleList(ObjectInfo, ObjectVertices, ObjectIndices);

		// 결합된 메시에 맞게 인덱스 조정
		uint32 VertexOffset = static_cast<uint32>(OutStaticMesh.Vertices.size());
		uint32 IndexOffset = static_cast<uint32>(OutStaticMesh.Indices.size());
		for (uint32& Index : ObjectIndices)
		{
			Index += VertexOffset;
		}

		// 최종 메시에 추가
		OutStaticMesh.Vertices.insert(OutStaticMesh.Vertices.end(), ObjectVertices.begin(), ObjectVertices.end());
		OutStaticMesh.Indices.insert(OutStaticMesh.Indices.end(), ObjectIndices.begin(), ObjectIndices.end());

		for (const FObjSectionInfo& SectionInfo : ObjectInfo.Sections)
		{
			if (SectionInfo.IndexCount <= 0)
			{
				continue;
			}

			int32 MaxCount = static_cast<int32>(ObjectIndices.size());
			if (SectionInfo.StartIndex < 0 || SectionInfo.StartIndex >= MaxCount)
			{
				continue;
			}

			int32 ClampedCount = std::min(SectionInfo.IndexCount, MaxCount - SectionInfo.StartIndex);
			if (ClampedCount <= 0)
			{
				continue;
			}

			FStaticMeshSection MeshSection;
			MeshSection.StartIndex = IndexOffset + SectionInfo.StartIndex;
			MeshSection.IndexCount = ClampedCount;
			MeshSection.MaterialSlotIndex = -1;
			MeshSection.MaterialName = SectionInfo.MaterialName;
			OutStaticMesh.Sections.push_back(MeshSection);
		}
	}

	if (OutStaticMesh.Sections.empty() && !OutStaticMesh.Indices.empty())
	{
		FStaticMeshSection DefaultSection;
		DefaultSection.StartIndex = 0;
		DefaultSection.IndexCount = static_cast<int32>(OutStaticMesh.Indices.size());
		DefaultSection.MaterialSlotIndex = -1;
		OutStaticMesh.Sections.push_back(DefaultSection);
	}

	return !OutStaticMesh.Vertices.empty() && !OutStaticMesh.Indices.empty();
}


bool FObjImporter::ImportStaticMesh(const FString& InFilePath, FStaticMesh& OutStaticMesh, TArray<FObjInfo>& OutObjectInfos)
{
	TArray<FObjInfo> ObjectInfos;

	if (!ImportObjFile(InFilePath, ObjectInfos))
	{
		return false;
	}

	OutObjectInfos = std::move(ObjectInfos);

	OutStaticMesh.PathFileName = InFilePath;
	return ConvertToStaticMesh(OutObjectInfos, OutStaticMesh);
}


void FObjImporter::ParseOBJLine(const FString& Line,
	FObjInfo& CurrentObject,
	TArray<FObjInfo>& AllObjects,
	TArray<FVector>& GlobalVertices,
	TArray<FVector2>& GlobalUVs,
	TArray<FVector>& GlobalNormals,
	int32& CurrentSectionIndex,
	FString& CurrentGroupName,
	FString& CurrentMaterialName,
	TMap<FString, FObjMaterialInfo>& MaterialLibrary,
	const FString& ObjDirectory)
{
	FString TrimmedLine = TrimString(Line);
	if (TrimmedLine.empty() || TrimmedLine[0] == '#')
	{
		return; // 빈 줄과 주석은 건너뜀
	}

	TArray<FString> Tokens;
	SplitString(TrimmedLine, ' ', Tokens);
	if (Tokens.empty())
	{
		return;
	}

	if (Tokens[0] == "v" && Tokens.size() >= 4)
	{
		// 정점 위치 - UE 좌표계로 변환
		FVector ObjPosition(
			std::stof(Tokens[1].c_str()),
			std::stof(Tokens[2].c_str()),
			std::stof(Tokens[3].c_str())
		);
		GlobalVertices.push_back(PositionToUEBasis(ObjPosition));
	}
	else if (Tokens[0] == "vt" && Tokens.size() >= 3)
	{
		// 텍스처 좌표 - UE 좌표계로 변환
		FVector2 ObjUV(
			std::stof(Tokens[1].c_str()),
			std::stof(Tokens[2].c_str())
		);
		GlobalUVs.push_back(UVToUEBasis(ObjUV));
	}
	else if (Tokens[0] == "vn" && Tokens.size() >= 4)
	{
		// 정점 노멀 - UE 좌표계로 변환
		FVector ObjNormal(
			std::stof(Tokens[1].c_str()),
			std::stof(Tokens[2].c_str()),
			std::stof(Tokens[3].c_str())
		);
		GlobalNormals.push_back(PositionToUEBasis(ObjNormal));
	}
	else if (Tokens[0] == "f" && Tokens.size() >= 4)
	{
		// 면 데이터
		FString FaceData = "";
		for (size_t i = 1; i < Tokens.size(); ++i)
		{
			if (i > 1)
			{
				FaceData += " ";
			}
			FaceData += Tokens[i];
		}

		int32 PreviousIndexCount = static_cast<int32>(CurrentObject.VertexIndexList.size());
		int32 AddedIndices = ParseFaceData(FaceData, CurrentObject);
		if (AddedIndices > 0)
		{
			if (CurrentSectionIndex < 0)
			{
				FString SectionName = !CurrentGroupName.empty() ? CurrentGroupName : CurrentObject.ObjectName;
				CurrentObject.Sections.emplace_back(SectionName, PreviousIndexCount);
				CurrentSectionIndex = static_cast<int32>(CurrentObject.Sections.size()) - 1;

				if (!CurrentMaterialName.empty())
				{
					CurrentObject.Sections[CurrentSectionIndex].MaterialName = CurrentMaterialName;
				}
			}

			if (CurrentSectionIndex >= 0 && CurrentSectionIndex < static_cast<int32>(CurrentObject.Sections.size()))
			{
				FObjSectionInfo& ActiveSection = CurrentObject.Sections[CurrentSectionIndex];
				if (ActiveSection.IndexCount == 0)
				{
					ActiveSection.StartIndex = PreviousIndexCount;
				}
				ActiveSection.IndexCount += AddedIndices;
			}
		}
	}
	else if (Tokens[0] == "o" && Tokens.size() > 1)
	{
		if (!AllObjects.empty() && AllObjects[0].ObjectName == "TempObject")
		{
			AllObjects[0].ObjectName = Tokens[1];
			CurrentGroupName = Tokens[1];
		}
		else
		{
			AllObjects.emplace_back(Tokens[1]);
		}

		CurrentSectionIndex = -1;
		CurrentMaterialName.clear();
	}
	else if (Tokens[0] == "g" && Tokens.size() > 1)
	{
		FString SectionName = Tokens[1];
		CurrentGroupName = SectionName;
		if (CurrentSectionIndex >= 0 && CurrentSectionIndex < static_cast<int32>(CurrentObject.Sections.size()))
		{
			FObjSectionInfo& ActiveSection = CurrentObject.Sections[CurrentSectionIndex];
			if (ActiveSection.IndexCount == 0)
			{
				ActiveSection.SectionName = SectionName;
				if (!CurrentMaterialName.empty())
				{
					ActiveSection.MaterialName = CurrentMaterialName;
				}
				return;
			}
		}

		int32 SectionStart = static_cast<int32>(CurrentObject.VertexIndexList.size());
		CurrentObject.Sections.emplace_back(SectionName, SectionStart);
		CurrentSectionIndex = static_cast<int32>(CurrentObject.Sections.size()) - 1;
		if (!CurrentMaterialName.empty())
		{
			CurrentObject.Sections[CurrentSectionIndex].MaterialName = CurrentMaterialName;
		}
	}
	else if ((Tokens[0] == "mtllib" || Tokens[0] == "matlib") && Tokens.size() > 1)
	{
		FString LibraryName = Tokens[1];
		FString LibraryPath = LibraryName;
		bool bIsAbsolute = (LibraryName.find(':') != FString::npos) || (!LibraryName.empty() && (LibraryName[0] == '/' || LibraryName[0] == '\\'));
		if (!bIsAbsolute)
		{
			LibraryPath = ObjDirectory + LibraryName;
		}
		ParseMaterialLibrary(LibraryPath, MaterialLibrary);
	}
	else if (Tokens[0] == "usemtl" && Tokens.size() > 1)
	{
		CurrentMaterialName = Tokens[1];
		auto MaterialIt = MaterialLibrary.find(CurrentMaterialName);
		if (MaterialIt != MaterialLibrary.end())
		{
			CurrentObject.MaterialInfos[CurrentMaterialName] = MaterialIt->second;
		}
		else
		{
			CurrentObject.MaterialInfos[CurrentMaterialName] = FObjMaterialInfo(CurrentMaterialName);
		}

		bool bCreateNewSection = false;
		if (CurrentSectionIndex < 0)
		{
			bCreateNewSection = true;
		}
		else if (CurrentSectionIndex >= 0 && CurrentSectionIndex < static_cast<int32>(CurrentObject.Sections.size()))
		{
			FObjSectionInfo& ActiveSection = CurrentObject.Sections[CurrentSectionIndex];
			if (!ActiveSection.MaterialName.empty() && ActiveSection.MaterialName != CurrentMaterialName)
			{
				bCreateNewSection = true;
			}
			else if (ActiveSection.IndexCount > 0 && ActiveSection.MaterialName != CurrentMaterialName)
			{
				bCreateNewSection = true;
			}
		}

		if (bCreateNewSection)
		{
			int32 SectionStart = static_cast<int32>(CurrentObject.VertexIndexList.size());
			FString SectionName = !CurrentGroupName.empty() ? CurrentGroupName : CurrentMaterialName;
			CurrentObject.Sections.emplace_back(SectionName, SectionStart);
			CurrentSectionIndex = static_cast<int32>(CurrentObject.Sections.size()) - 1;
		}

		if (CurrentSectionIndex >= 0 && CurrentSectionIndex < static_cast<int32>(CurrentObject.Sections.size()))
		{
			CurrentObject.Sections[CurrentSectionIndex].MaterialName = CurrentMaterialName;
		}
	}
}


int32 FObjImporter::ParseFaceData(const FString& FaceData, FObjInfo& CurrentObject)
{
	TArray<FString> FaceVertices;
	SplitString(FaceData, ' ', FaceVertices);

	size_t VertexCountBefore = CurrentObject.VertexIndexList.size();

	// 면을 삼각형으로 변환 (면이 최소 삼각형이라고 가정)
	for (size_t i = 1; i < FaceVertices.size() - 1; ++i)
	{
		// 삼각형의 각 정점을 파싱
		TArray<FString> VertexComponents[3];
		SplitString(FaceVertices[0], '/', VertexComponents[0]);
		SplitString(FaceVertices[i], '/', VertexComponents[1]);
		SplitString(FaceVertices[i + 1], '/', VertexComponents[2]);

		for (int j = 0; j < 3; ++j)
		{
			// 삼각형의 각 정점에 대한 인덱스 추가
			if (!VertexComponents[j].empty() && !VertexComponents[j][0].empty())
			{
				CurrentObject.VertexIndexList.push_back(std::stoi(VertexComponents[j][0].c_str()) - 1); // OBJ 인덱스는 1부터 시작
			}

			if (VertexComponents[j].size() > 1 && !VertexComponents[j][1].empty())
			{
				CurrentObject.UVIndexList.push_back(std::stoi(VertexComponents[j][1].c_str()) - 1);
			}

			if (VertexComponents[j].size() > 2 && !VertexComponents[j][2].empty())
			{
				CurrentObject.NormalIndexList.push_back(std::stoi(VertexComponents[j][2].c_str()) - 1);
			}
		}
	}

	size_t VertexCountAfter = CurrentObject.VertexIndexList.size();
	return static_cast<int32>(VertexCountAfter - VertexCountBefore);
}


void FObjImporter::ConvertToTriangleList(const FObjInfo& ObjectInfo,
	TArray<FVertex>& OutVertices,
	TArray<uint32>& OutIndices)
{
	OutVertices.clear();
	OutIndices.clear();

	// 커스텀 해시 함수 정의
	struct VertexHash
	{
		std::size_t operator()(const std::tuple<FVector, FVector2, FVector>& v) const
		{
			const auto& pos = std::get<0>(v);
			const auto& uv = std::get<1>(v);
			const auto& normal = std::get<2>(v);

			// 간단한 해시 조합
			std::size_t h1 = std::hash<float>{}(pos.X) ^ (std::hash<float>{}(pos.Y) << 1) ^ (std::hash<float>{}(pos.Z) << 2);
			std::size_t h2 = std::hash<float>{}(uv.X) ^ (std::hash<float>{}(uv.Y) << 1);
			std::size_t h3 = std::hash<float>{}(normal.X) ^ (std::hash<float>{}(normal.Y) << 1) ^ (std::hash<float>{}(normal.Z) << 2);

			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};

	// 커스텀 동등성 비교자 정의
	struct VertexEqual
	{
		bool operator()(const std::tuple<FVector, FVector2, FVector>& lhs,
		               const std::tuple<FVector, FVector2, FVector>& rhs) const
		{
			const auto& pos1 = std::get<0>(lhs);
			const auto& uv1 = std::get<1>(lhs);
			const auto& normal1 = std::get<2>(lhs);

			const auto& pos2 = std::get<0>(rhs);
			const auto& uv2 = std::get<1>(rhs);
			const auto& normal2 = std::get<2>(rhs);

			// float 비교를 위한 epsilon 사용
			const float epsilon = 1e-6f;

			return (abs(pos1.X - pos2.X) < epsilon && abs(pos1.Y - pos2.Y) < epsilon && abs(pos1.Z - pos2.Z) < epsilon) &&
			       (abs(uv1.X - uv2.X) < epsilon && abs(uv1.Y - uv2.Y) < epsilon) &&
			       (abs(normal1.X - normal2.X) < epsilon && abs(normal1.Y - normal2.Y) < epsilon && abs(normal1.Z - normal2.Z) < epsilon);
		}
	};

	// 고유 정점을 추적하기 위한 맵 (정점 데이터 → 인덱스)
	std::unordered_map<std::tuple<FVector, FVector2, FVector>, uint32, VertexHash, VertexEqual> VertexMap;

	size_t NumTriangles = ObjectInfo.VertexIndexList.size() / 3;

	for (size_t i = 0; i < NumTriangles; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			size_t Index = i * 3 + j;
			FVertex Vertex;

			// 위치
			if (Index < ObjectInfo.VertexIndexList.size())
			{
				uint32 VertexIndex = ObjectInfo.VertexIndexList[Index];
				if (VertexIndex < ObjectInfo.VertexList.size())
				{
					Vertex.Position = ObjectInfo.VertexList[VertexIndex];
				}
			}

			// UV
			if (Index < ObjectInfo.UVIndexList.size())
			{
				uint32 UVIndex = ObjectInfo.UVIndexList[Index];
				if (UVIndex < ObjectInfo.UVList.size())
				{
					Vertex.TextureCoord = ObjectInfo.UVList[UVIndex];
				}
			}

			// 노멀
			if (Index < ObjectInfo.NormalIndexList.size())
			{
				uint32 NormalIndex = ObjectInfo.NormalIndexList[Index];
				if (NormalIndex < ObjectInfo.NormalList.size())
				{
					Vertex.Normal = ObjectInfo.NormalList[NormalIndex];
				}
			}

			// 정점 고유성 확인을 위한 키 생성
			auto VertexKey = std::make_tuple(Vertex.Position, Vertex.TextureCoord, Vertex.Normal);

			// 이미 존재하는 정점인지 확인
			auto It = VertexMap.find(VertexKey);
			if (It != VertexMap.end())
			{
				// 기존 정점 인덱스 사용
				OutIndices.push_back(It->second);
			}
			else
			{
				// 새로운 정점 추가
				uint32 NewVertexIndex = static_cast<uint32>(OutVertices.size());
				OutVertices.push_back(Vertex);
				VertexMap[VertexKey] = NewVertexIndex;
				OutIndices.push_back(NewVertexIndex);
			}
		}
	}
}

FString FObjImporter::TrimString(const FString& String)
{
	if (String.empty())
	{
		return String;
	}

	size_t Start = 0;
	size_t End = String.length() - 1;

	while (Start <= End && (String[Start] == ' ' || String[Start] == '\t' || String[Start] == '\r' || String[Start] == '\n'))
	{
		Start++;
	}

	while (End > Start && (String[End] == ' ' || String[End] == '\t' || String[End] == '\r' || String[End] == '\n'))
	{
		End--;
	}

	return String.substr(Start, End - Start + 1);
}

void FObjImporter::SplitString(const FString& String, char Delimiter, TArray<FString>& OutTokens)
{
	OutTokens.clear();
	std::stringstream StringStream(String);
	std::string Token;

	while (std::getline(StringStream, Token, Delimiter))
	{
		FString TrimmedToken = TrimString(FString(Token.c_str()));
		if (!TrimmedToken.empty())
		{
			OutTokens.push_back(TrimmedToken);
		}
	}
}

FVector FObjImporter::PositionToUEBasis(const FVector& InVector)
{
	return FVector(InVector.X, -InVector.Y, InVector.Z);
}

FVector2 FObjImporter::UVToUEBasis(const FVector2& InVector)
{
	return FVector2(InVector.X, 1.0f - InVector.Y);
}
