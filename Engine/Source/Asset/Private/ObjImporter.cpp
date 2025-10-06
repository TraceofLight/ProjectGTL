#include "pch.h"
#include "Asset/Public/ObjImporter.h"
#include <algorithm>
#include <utility>

bool FObjImporter::ImportObjFile(const FString& InFilePath, TArray<FObjInfo>& OutObjectInfos)
{
	std::ifstream File(InFilePath);
	if (!File.is_open())
	{
		return false;
	}

	OutObjectInfos.IsEmpty();

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

	// 단일 오브젝트로 통합 파싱
	FObjInfo SingleObject("CombinedObject");
	TMap<FString, FObjMaterialInfo> MaterialLibrary;

	// 현재 섹션 상태
	int32 CurrentSectionIndex = -1; // 활성 섹션 인덱스(없으면 -1)
	FString CurrentMaterialName; // 마지막 usemtl 이름(없으면 빈 문자열)

	FString ObjLine;
	while (std::getline(File, ObjLine))
	{
		ParseOBJLine(ObjLine,
		             SingleObject,
		             GlobalVertices,
		             GlobalUVs,
		             GlobalNormals,
		             CurrentSectionIndex,
		             CurrentMaterialName,
		             MaterialLibrary,
		             ObjDirectory);
	}

	File.close();

	// 전역 배열을 단일 오브젝트에 바인딩
	SingleObject.VertexList = GlobalVertices;
	SingleObject.UVList = GlobalUVs;
	SingleObject.NormalList = GlobalNormals;

	// 섹션 후처리: 하나도 없는데 인덱스가 있으면 디폴트 섹션 생성(MaterialName = "")
	// TODO: 이렇게 하면 처음엔 머티리얼 섹션 없이 f 나오다가 중간부터 usemtl 나오는 경우 처리 안 되는 섹션 발생
	// 렌더링을 섹션 단위로 돌 것이기 때문에 섹션들을 전부 모았을 때 모든 인덱스 범위가 커버되어야 함.
	// 파싱 시작 때 머티리얼 네임 없는 섹션 하나 만들어두고 시작하도록 수정 필요
	if (SingleObject.Sections.IsEmpty() && !SingleObject.VertexIndexList.IsEmpty())
	{
		FObjSectionInfo DefaultSection("", 0);
		DefaultSection.IndexCount = static_cast<int32>(SingleObject.VertexIndexList.Num());
		SingleObject.Sections.Add(DefaultSection);
	}

	// 이 OBJ에서 참조한 머티리얼들 기록
	SingleObject.MaterialInfos = MaterialLibrary;

	// 결과: 단 하나의 FObjInfo만 반환
	OutObjectInfos.IsEmpty();
	OutObjectInfos.Add(std::move(SingleObject));
	return true;
}

bool FObjImporter::ParseMaterialLibrary(const FString& InMTLFilePath,
                                        TMap<FString, FObjMaterialInfo>& OutMaterialLibrary)
{
	std::ifstream File(InMTLFilePath);
	if (!File.is_open())
	{
		return false;
	}
	UE_LOG("Loading MTL file: %s", InMTLFilePath.c_str());
	FObjMaterialInfo* CurrentMaterial = nullptr;

	FString MTLLine;
	while (std::getline(File, MTLLine))
	{
		MTLLine = TrimString(MTLLine);

		if (MTLLine.empty() || MTLLine[0] == '#')
		{
			continue; // 빈 줄과 주석은 건너뜀
		}

		TArray<FString> Tokens;
		SplitString(MTLLine, ' ', Tokens);

		if (Tokens.IsEmpty())
		{
			continue;
		}

		if (Tokens[0] == "newmtl" && Tokens.Num() > 1)
		{
			// 새 재질
			OutMaterialLibrary[Tokens[1]] = FObjMaterialInfo(Tokens[1]);
			CurrentMaterial = &OutMaterialLibrary[Tokens[1]];
		}
		else if (CurrentMaterial != nullptr)
		{
			if (Tokens[0] == "map_Kd" && Tokens.Num() > 1)
			{
				CurrentMaterial->DiffuseTexturePath = Tokens[1];
			}
			else if (Tokens[0] == "map_Kn" && Tokens.Num() > 1)
			{
				CurrentMaterial->NormalTexturePath = Tokens[1];
			}
			else if (Tokens[0] == "map_Ks" && Tokens.Num() > 1)
			{
				CurrentMaterial->SpecularTexturePath = Tokens[1];
			}
			else if (Tokens[0] == "Ka" && Tokens.Num() > 3)
			{
				CurrentMaterial->AmbientColorScalar = FVector(
					std::stof(Tokens[1]),
					std::stof(Tokens[2]),
					std::stof(Tokens[3])
				);
			}
			else if (Tokens[0] == "Kd" && Tokens.Num() > 3)
			{
				CurrentMaterial->DiffuseColorScalar = FVector(
					std::stof(Tokens[1]),
					std::stof(Tokens[2]),
					std::stof(Tokens[3])
				);
			}
			else if (Tokens[0] == "Ks" && Tokens.Num() > 3)
			{
				CurrentMaterial->SpecularColorScalar = FVector(
					std::stof(Tokens[1]),
					std::stof(Tokens[2]),
					std::stof(Tokens[3])
				);
			}
			else if (Tokens[0] == "Ns" && Tokens.Num() > 1)
			{
				CurrentMaterial->ShininessScalar = std::stof(Tokens[1]);
			}
			else if (Tokens[0] == "d" && Tokens.Num() > 1)
			{
				CurrentMaterial->TransparencyScalar = std::stof(Tokens[1]);
			}
		}
	}

	File.close();
	return true;
}

bool FObjImporter::ConvertToStaticMesh(const TArray<FObjInfo>& InObjectInfos, FStaticMesh& OutStaticMesh)
{
	if (InObjectInfos.IsEmpty())
	{
		return false;
	}

	OutStaticMesh.Vertices.Empty();
	OutStaticMesh.Indices.Empty();
	OutStaticMesh.Sections.Empty();

	// 모든 객체를 하나의 스태틱 메시로 결합
	for (const FObjInfo& ObjectInfo : InObjectInfos)
	{
		TArray<FVertex> ObjectVertices;
		TArray<uint32> ObjectIndices;

		ConvertToTriangleList(ObjectInfo, ObjectVertices, ObjectIndices);

		// 결합된 메시에 맞게 인덱스 조정
		uint32 VertexOffset = static_cast<uint32>(OutStaticMesh.Vertices.Num());
		uint32 IndexOffset = static_cast<uint32>(OutStaticMesh.Indices.Num());
		for (uint32& Index : ObjectIndices)
		{
			Index += VertexOffset;
		}

		// 최종 메시에 추가
		OutStaticMesh.Vertices.Append(ObjectVertices);
		OutStaticMesh.Indices.Append(ObjectIndices);

		for (const FObjSectionInfo& SectionInfo : ObjectInfo.Sections)
		{
			if (SectionInfo.IndexCount <= 0)
			{
				continue;
			}

			int32 MaxCount = static_cast<int32>(ObjectIndices.Num());
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
			MeshSection.StartIndex = static_cast<uint32>(IndexOffset) + SectionInfo.StartIndex;
			MeshSection.IndexCount = ClampedCount;
			MeshSection.MaterialSlotIndex = -1;
			MeshSection.MaterialName = SectionInfo.MaterialName;
			OutStaticMesh.Sections.Add(MeshSection);
		}
	}

	if (OutStaticMesh.Sections.IsEmpty() && !OutStaticMesh.Indices.IsEmpty())
	{
		FStaticMeshSection DefaultSection;
		DefaultSection.StartIndex = 0;
		DefaultSection.IndexCount = static_cast<int32>(OutStaticMesh.Indices.Num());
		DefaultSection.MaterialSlotIndex = -1;
		OutStaticMesh.Sections.Add(DefaultSection);
	}

	return !OutStaticMesh.Vertices.IsEmpty() && !OutStaticMesh.Indices.IsEmpty();
}


bool FObjImporter::ImportStaticMesh(const FString& InFilePath, FStaticMesh& OutStaticMesh,
                                    TArray<FObjInfo>& OutObjectInfos)
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
                                FObjInfo& ObjectInfo,
                                TArray<FVector>& GlobalVertices,
                                TArray<FVector2>& GlobalUVs,
                                TArray<FVector>& GlobalNormals,
                                int32& CurrentSectionIndex,
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
	if (Tokens.IsEmpty())
	{
		return;
	}

	// --- Geometry Data ---
	if (Tokens[0] == "v" && Tokens.Num() >= 4)
	{
		// 정점 위치 - UE 좌표계로 변환
		FVector ObjPosition(
			std::stof(Tokens[1]),
			std::stof(Tokens[2]),
			std::stof(Tokens[3])
		);
		GlobalVertices.Add(PositionToUEBasis(ObjPosition));
	}
	else if (Tokens[0] == "vt" && Tokens.Num() >= 3)
	{
		// 텍스처 좌표 - UE 좌표계로 변환
		FVector2 ObjUV(
			std::stof(Tokens[1]),
			std::stof(Tokens[2])
		);
		GlobalUVs.Add(UVToUEBasis(ObjUV));
	}
	else if (Tokens[0] == "vn" && Tokens.Num() >= 4)
	{
		// 정점 노멀 - UE 좌표계로 변환
		FVector ObjNormal(
			std::stof(Tokens[1]),
			std::stof(Tokens[2]),
			std::stof(Tokens[3])
		);
		GlobalNormals.Add(PositionToUEBasis(ObjNormal));
	}

	// --- Face Data ---
	else if (Tokens[0] == "f" && Tokens.Num() >= 4)
	{
		FString FaceData;
		for (size_t i = 1; i < Tokens.Num(); ++i)
		{
			if (i > 1)
			{
				FaceData += " ";
			}
			FaceData += Tokens[i];
		}

		int32 PreviousIndexCount = static_cast<int32>(ObjectInfo.VertexIndexList.Num());
		int32 AddedIndices = ParseFaceData(FaceData, ObjectInfo);
		if (AddedIndices > 0)
		{
			// 활성 섹션이 없으면 디폴트 섹션 생성
			if (CurrentSectionIndex < 0)
			{
				// 디폴트 섹션 (MaterialName = "")
				FObjSectionInfo DefaultSection("", PreviousIndexCount);
				DefaultSection.IndexCount = 0; // 누적 방식
				ObjectInfo.Sections.Add(DefaultSection);
				CurrentSectionIndex = static_cast<int32>(ObjectInfo.Sections.Num()) - 1;
			}

			// 활성 섹션에 인덱스 개수 누적 (디폴트 섹션이든 usemtl 섹션이든 상관없이)
			if (CurrentSectionIndex >= 0 && CurrentSectionIndex < ObjectInfo.Sections.Num())
			{
				ObjectInfo.Sections[CurrentSectionIndex].IndexCount += AddedIndices;
			}
		}
	}

	// --- Material Data ---
	else if ((Tokens[0] == "mtllib" || Tokens[0] == "matlib") && Tokens.Num() > 1)
	{
		const FString& LibraryName = Tokens[1];
		FString LibraryPath = LibraryName;
		bool bIsAbsolute = (LibraryName.find(':') != FString::npos) || (!LibraryName.empty() && (LibraryName[0] == '/'
			|| LibraryName[0] == '\\'));
		if (!bIsAbsolute)
		{
			LibraryPath = ObjDirectory + LibraryName;
		}
		ParseMaterialLibrary(LibraryPath, MaterialLibrary);
	}
	else if (Tokens[0] == "usemtl" && Tokens.Num() > 1)
	{
		CurrentMaterialName = Tokens[1];

		FObjMaterialInfo* FoundMaterialInfo = MaterialLibrary.Find(CurrentMaterialName);

		if (FoundMaterialInfo)
		{
			ObjectInfo.MaterialInfos.Add(CurrentMaterialName, *FoundMaterialInfo);
		}
		else
		{
			ObjectInfo.MaterialInfos.Add(CurrentMaterialName, FObjMaterialInfo(CurrentMaterialName));
		}

		// 새 섹션 시작: 이후 f 인덱스가 여기에 누적됨
		const int32 start = ObjectInfo.VertexIndexList.Num();
		FObjSectionInfo NewSec(CurrentMaterialName, start);
		NewSec.IndexCount = 0;
		ObjectInfo.Sections.Add(NewSec);
		CurrentSectionIndex = ObjectInfo.Sections.Num() - 1;
	}
}


int32 FObjImporter::ParseFaceData(const FString& FaceData, FObjInfo& CurrentObject)
{
	TArray<FString> FaceVertices;
	SplitString(FaceData, ' ', FaceVertices);

	size_t VertexCountBefore = CurrentObject.VertexIndexList.Num();

	// 면을 삼각형으로 변환 (면이 최소 삼각형이라고 가정)
	for (size_t i = 1; i < FaceVertices.Num() - 1; ++i)
	{
		// 삼각형의 각 정점을 파싱
		TArray<FString> VertexComponents[3];
		SplitString(FaceVertices[0], '/', VertexComponents[0]);
		SplitString(FaceVertices[i], '/', VertexComponents[1]);
		SplitString(FaceVertices[i + 1], '/', VertexComponents[2]);

		for (int j = 0; j < 3; ++j)
		{
			// 삼각형의 각 정점에 대한 인덱스 추가
			if (!VertexComponents[j].IsEmpty() && !VertexComponents[j][0].empty())
			{
				CurrentObject.VertexIndexList.Add(std::stoi(VertexComponents[j][0]) - 1); // OBJ 인덱스는 1부터 시작
			}

			if (VertexComponents[j].Num() > 1 && !VertexComponents[j][1].empty())
			{
				CurrentObject.UVIndexList.Add(std::stoi(VertexComponents[j][1]) - 1);
			}

			if (VertexComponents[j].Num() > 2 && !VertexComponents[j][2].empty())
			{
				CurrentObject.NormalIndexList.Add(std::stoi(VertexComponents[j][2]) - 1);
			}
		}
	}

	size_t VertexCountAfter = CurrentObject.VertexIndexList.Num();
	return static_cast<int32>(VertexCountAfter - VertexCountBefore);
}


void FObjImporter::ConvertToTriangleList(const FObjInfo& ObjectInfo,
                                         TArray<FVertex>& OutVertices,
                                         TArray<uint32>& OutIndices)
{
	OutVertices.Empty();
	OutIndices.Empty();

	// 커스텀 해시 함수 정의
	struct VertexHash
	{
		std::size_t operator()(const std::tuple<FVector, FVector2, FVector>& v) const
		{
			const auto& pos = std::get<0>(v);
			const auto& uv = std::get<1>(v);
			const auto& normal = std::get<2>(v);

			// 간단한 해시 조합
			std::size_t h1 = std::hash<float>{}(pos.X) ^ (std::hash<float>{}(pos.Y) << 1) ^ (std::hash<float>{}(pos.Z)
				<< 2);
			std::size_t h2 = std::hash<float>{}(uv.X) ^ (std::hash<float>{}(uv.Y) << 1);
			std::size_t h3 = std::hash<float>{}(normal.X) ^ (std::hash<float>{}(normal.Y) << 1) ^ (std::hash<float>
				{}(normal.Z) << 2);

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
			constexpr float epsilon = 1e-6f;

			return (abs(pos1.X - pos2.X) < epsilon && abs(pos1.Y - pos2.Y) < epsilon && abs(pos1.Z - pos2.Z) < epsilon)
				&&
				(abs(uv1.X - uv2.X) < epsilon && abs(uv1.Y - uv2.Y) < epsilon) &&
				(abs(normal1.X - normal2.X) < epsilon && abs(normal1.Y - normal2.Y) < epsilon && abs(
					normal1.Z - normal2.Z) < epsilon);
		}
	};

	// 고유 정점을 추적하기 위한 맵 (정점 데이터 → 인덱스)
	std::unordered_map<std::tuple<FVector, FVector2, FVector>, uint32, VertexHash, VertexEqual> VertexMap;

	size_t NumTriangles = ObjectInfo.VertexIndexList.Num() / 3;

	for (size_t i = 0; i < NumTriangles; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			size_t Index = i * 3 + j;
			FVertex Vertex;

			// 위치
			if (Index < ObjectInfo.VertexIndexList.Num())
			{
				uint32 VertexIndex = ObjectInfo.VertexIndexList[Index];
				if (VertexIndex < ObjectInfo.VertexList.Num())
				{
					Vertex.Position = ObjectInfo.VertexList[VertexIndex];
				}
			}

			// UV
			if (Index < ObjectInfo.UVIndexList.Num())
			{
				uint32 UVIndex = ObjectInfo.UVIndexList[Index];
				if (UVIndex < ObjectInfo.UVList.Num())
				{
					Vertex.TextureCoord = ObjectInfo.UVList[UVIndex];
				}
			}

			// 노멀
			if (Index < ObjectInfo.NormalIndexList.Num())
			{
				uint32 NormalIndex = ObjectInfo.NormalIndexList[Index];
				if (NormalIndex < ObjectInfo.NormalList.Num())
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
				OutIndices.Add(It->second);
			}
			else
			{
				// 새로운 정점 추가
				uint32 NewVertexIndex = static_cast<uint32>(OutVertices.Num());
				OutVertices.Add(Vertex);
				VertexMap[VertexKey] = NewVertexIndex;
				OutIndices.Add(NewVertexIndex);
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

	while (Start <= End && (String[Start] == ' ' || String[Start] == '\t' || String[Start] == '\r' || String[Start] ==
		'\n'))
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
	OutTokens.Empty();
	std::stringstream StringStream(String);
	std::string Token;

	while (std::getline(StringStream, Token, Delimiter))
	{
		FString TrimmedToken = TrimString(FString(Token));
		if (!TrimmedToken.empty())
		{
			OutTokens.Add(TrimmedToken);
		}
	}
}

FVector FObjImporter::PositionToUEBasis(const FVector& InVector)
{
	return {InVector.X, -InVector.Y, InVector.Z};
}

FVector2 FObjImporter::UVToUEBasis(const FVector2& InVector)
{
	return {InVector.X, 1.0f - InVector.Y};
}
