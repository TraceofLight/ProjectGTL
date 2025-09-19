#include "pch.h"
#include "Asset/Public/ObjImporter.h"

bool FObjImporter::ImportOBJFile(const FString& FilePath, TArray<FObjInfo>& OutObjectInfos)
{
	std::ifstream File(FilePath.c_str());
	if (!File.is_open())
	{
		return false;
	}

	OutObjectInfos.clear();

	// Global data arrays (OBJ files have global vertex/UV/normal lists)
	TArray<FVector> GlobalVertices;
	TArray<FVector2> GlobalUVs;
	TArray<FVector> GlobalNormals;

	// Start with a default object
	FObjInfo CurrentObject("DefaultObject");
	OutObjectInfos.push_back(CurrentObject);

	std::string Line;
	while (std::getline(File, Line))
	{
		FString ObjLine(Line.c_str());
		ParseOBJLine(ObjLine, OutObjectInfos.back(), OutObjectInfos, GlobalVertices, GlobalUVs, GlobalNormals);
	}

	File.close();

	// Copy global data to each object for easier processing
	for (FObjInfo& ObjectInfo : OutObjectInfos)
	{
		ObjectInfo.VertexList = GlobalVertices;
		ObjectInfo.UVList = GlobalUVs;
		ObjectInfo.NormalList = GlobalNormals;
	}

	return true;
}

bool FObjImporter::ParseMaterialLibrary(const FString& MTLFilePath, TArray<FObjMaterialInfo>& OutMaterials)
{
	std::ifstream File(MTLFilePath.c_str());
	if (!File.is_open())
	{
		return false;
	}

	OutMaterials.clear();
	FObjMaterialInfo* CurrentMaterial = nullptr;

	std::string Line;
	while (std::getline(File, Line))
	{
		FString MTLLine = TrimString(FString(Line.c_str()));

		if (MTLLine.empty() || MTLLine[0] == '#')
		{
			continue; // Skip empty lines and comments
		}

		TArray<FString> Tokens;
		SplitString(MTLLine, ' ', Tokens);

		if (Tokens.empty())
		{
			continue;
		}

		if (Tokens[0] == "newmtl" && Tokens.size() > 1)
		{
			// New material
			OutMaterials.emplace_back(Tokens[1]);
			CurrentMaterial = &OutMaterials.back();
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

bool FObjImporter::ConvertToStaticMesh(const TArray<FObjInfo>& ObjectInfos, FStaticMesh& OutStaticMesh)
{
	if (ObjectInfos.empty())
	{
		return false;
	}

	OutStaticMesh.Vertices.clear();
	OutStaticMesh.Indices.clear();

	// Combine all objects into one static mesh
	for (const FObjInfo& ObjectInfo : ObjectInfos)
	{
		TArray<FVertex> ObjectVertices;
		TArray<uint32> ObjectIndices;

		ConvertToTriangleList(ObjectInfo, ObjectVertices, ObjectIndices);

		// Adjust indices for the combined mesh
		uint32 VertexOffset = static_cast<uint32>(OutStaticMesh.Vertices.size());
		for (uint32& Index : ObjectIndices)
		{
			Index += VertexOffset;
		}

		// Append to final mesh
		OutStaticMesh.Vertices.insert(OutStaticMesh.Vertices.end(), ObjectVertices.begin(), ObjectVertices.end());
		OutStaticMesh.Indices.insert(OutStaticMesh.Indices.end(), ObjectIndices.begin(), ObjectIndices.end());
	}

	return !OutStaticMesh.Vertices.empty() && !OutStaticMesh.Indices.empty();
}

bool FObjImporter::ImportStaticMesh(const FString& FilePath, FStaticMesh& OutStaticMesh)
{
	TArray<FObjInfo> ObjectInfos;

	if (!ImportOBJFile(FilePath, ObjectInfos))
	{
		return false;
	}

	OutStaticMesh.PathFileName = FilePath;
	return ConvertToStaticMesh(ObjectInfos, OutStaticMesh);
}

void FObjImporter::ParseOBJLine(const FString& Line,
	FObjInfo& CurrentObject,
	TArray<FObjInfo>& AllObjects,
	TArray<FVector>& GlobalVertices,
	TArray<FVector2>& GlobalUVs,
	TArray<FVector>& GlobalNormals)
{
	FString TrimmedLine = TrimString(Line);

	if (TrimmedLine.empty() || TrimmedLine[0] == '#')
	{
		return; // Skip empty lines and comments
	}

	TArray<FString> Tokens;
	SplitString(TrimmedLine, ' ', Tokens);

	if (Tokens.empty())
	{
		return;
	}

	if (Tokens[0] == "v" && Tokens.size() >= 4)
	{
		// Vertex position - convert to UE coordinate system
		FVector ObjPosition(
			std::stof(Tokens[1].c_str()),
			std::stof(Tokens[2].c_str()),
			std::stof(Tokens[3].c_str())
		);
		GlobalVertices.push_back(PositionToUEBasis(ObjPosition));
	}
	else if (Tokens[0] == "vt" && Tokens.size() >= 3)
	{
		// Texture coordinate - convert to UE coordinate system
		FVector2 ObjUV(
			std::stof(Tokens[1].c_str()),
			std::stof(Tokens[2].c_str())
		);
		GlobalUVs.push_back(UVToUEBasis(ObjUV));
	}
	else if (Tokens[0] == "vn" && Tokens.size() >= 4)
	{
		// Vertex normal - convert to UE coordinate system
		FVector ObjNormal(
			std::stof(Tokens[1].c_str()),
			std::stof(Tokens[2].c_str()),
			std::stof(Tokens[3].c_str())
		);
		GlobalNormals.push_back(PositionToUEBasis(ObjNormal));
	}
	else if (Tokens[0] == "f" && Tokens.size() >= 4)
	{
		// Face data
		FString FaceData = "";
		for (size_t i = 1; i < Tokens.size(); ++i)
		{
			if (i > 1) FaceData += " ";
			FaceData += Tokens[i];
		}
		ParseFaceData(FaceData, CurrentObject);
	}
	else if ((Tokens[0] == "o" || Tokens[0] == "g") && Tokens.size() > 1)
	{
		// New object or group
		AllObjects.emplace_back(Tokens[1]);
	}
}

void FObjImporter::ParseFaceData(const FString& FaceData, FObjInfo& CurrentObject)
{
	TArray<FString> FaceVertices;
	SplitString(FaceData, ' ', FaceVertices);

	// Convert face to triangles (assuming face is at least a triangle)
	for (size_t i = 1; i < FaceVertices.size() - 1; ++i)
	{
		// Parse each vertex of the triangle
		TArray<FString> VertexComponents[3];
		SplitString(FaceVertices[0], '/', VertexComponents[0]);
		SplitString(FaceVertices[i], '/', VertexComponents[1]);
		SplitString(FaceVertices[i + 1], '/', VertexComponents[2]);

		// Add indices for each vertex of the triangle
		for (int j = 0; j < 3; ++j)
		{
			if (!VertexComponents[j].empty() && !VertexComponents[j][0].empty())
			{
				CurrentObject.VertexIndexList.push_back(std::stoi(VertexComponents[j][0].c_str()) - 1); // OBJ indices are 1-based
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
}

void FObjImporter::ConvertToTriangleList(const FObjInfo& ObjectInfo,
	TArray<FVertex>& OutVertices,
	TArray<uint32>& OutIndices)
{
	OutVertices.clear();
	OutIndices.clear();

	size_t NumTriangles = ObjectInfo.VertexIndexList.size() / 3;

	for (size_t i = 0; i < NumTriangles; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			size_t Index = i * 3 + j;
			FVertex Vertex;

			// Position
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

			// Normal
			if (Index < ObjectInfo.NormalIndexList.size())
			{
				uint32 NormalIndex = ObjectInfo.NormalIndexList[Index];
				if (NormalIndex < ObjectInfo.NormalList.size())
				{
					Vertex.Normal = ObjectInfo.NormalList[NormalIndex];
				}
			}

			OutVertices.push_back(Vertex);
			OutIndices.push_back(static_cast<uint32>((OutVertices.size() - 1)));
		}
	}
}

FString FObjImporter::TrimString(const FString& Str)
{
	if (Str.empty())
	{
		return Str;
	}

	size_t Start = 0;
	size_t End = Str.length() - 1;

	while (Start <= End && (Str[Start] == ' ' || Str[Start] == '\t' || Str[Start] == '\r' || Str[Start] == '\n'))
	{
		Start++;
	}

	while (End > Start && (Str[End] == ' ' || Str[End] == '\t' || Str[End] == '\r' || Str[End] == '\n'))
	{
		End--;
	}

	return Str.substr(Start, End - Start + 1);
}

void FObjImporter::SplitString(const FString& Str, char Delimiter, TArray<FString>& OutTokens)
{
	OutTokens.clear();
	std::stringstream ss(Str);
	std::string Token;

	while (std::getline(ss, Token, Delimiter))
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
