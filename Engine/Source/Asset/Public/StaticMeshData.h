#pragma once
#include "Global/Types.h"
#include "Global/Vector.h"
#include "Global/CoreTypes.h"

/**
 * @brief 스태틱 메시 내 객체의 재질 정보
 * 텍스처 경로 및 조명 매개변수를 포함한 모든 재질 속성을 포함.
 */
struct FObjMaterialInfo
{
	FString MaterialName;
	FString DiffuseTexturePath;
	FString NormalTexturePath;
	FString SpecularTexturePath;
	FVector AmbientColorScalar = FVector(0.2f, 0.2f, 0.2f);
	FVector DiffuseColorScalar = FVector(0.8f, 0.8f, 0.8f);
	FVector SpecularColorScalar = FVector(1.0f, 1.0f, 1.0f);
	float ShininessScalar = 32.0f;
	float TransparencyScalar = 1.0f;

	FObjMaterialInfo() = default;

	/**
	 * @brief 재질 이름으로 생성하는 생성자
	 * @param InMaterialName 생성할 재질의 이름
	 */
	FObjMaterialInfo(const FString& InMaterialName)
		: MaterialName(InMaterialName)
	{
	}
};

/**
 * @brief 스태틱 메시 내의 단일 객체에 대한 원시 데이터 구조
 *
 * OBJ 파일에서 파싱된 각 데이터 유형에 대한 별도의 배열을 포함.
 * 이 구조는 최종 정점 형식으로 처리되기 전의 원시 OBJ 데이터를 보유.
 * OBJ 로더의 중간 파싱 구조와 유사.
 */
struct FObjInfo
{
	FString ObjectName;							// 객체 이름 (OBJ 'o' 또는 'g' 명령어에서 가져옴)
	TArray<FVector> VertexList;					// OBJ 'v' 명령어의 원시 정점 위치
	TArray<FVector2> UVList;					// OBJ 'vt' 명령어의 원시 UV 좌표
	TArray<FVector> NormalList;					// OBJ 'vn' 명령어의 원시 노멀 벡터
	TArray<uint32> VertexIndexList;				// OBJ 'f' 명령어의 정점 인덱스 (VertexList 참조)
	TArray<uint32> UVIndexList;					// OBJ 'f' 명령어의 UV 인덱스 (UVList 참조)
	TArray<uint32> NormalIndexList;				// OBJ 'f' 명령어의 노멀 인덱스 (NormalList 참조)
	TArray<FObjMaterialInfo> MaterialList;	// 이 객체에서 사용하는 재질 목록
	TArray<FString> TextureList;				// 이 객체에서 사용하는 텍스처 파일 경로 목록

	FObjInfo() = default;

	/**
	 * @brief 객체 이름으로 생성하는 생성자
	 * @param InObjectName 생성할 객체의 이름
	 */
	FObjInfo(const FString& InObjectName)
		: ObjectName(InObjectName)
	{
	}
};

/**
 * @brief 스태틱 메시 데이터 구조 (언리얼의 `FStaticMeshLODResources`에 해당)
 * @note 이 구조는 렌더링을 위해 최종 처리된 데이터를 보유.
 * GPU 버퍼 생성을 위한 쿠킹된 정점 데이터와 인덱스를 포함.
 */
struct FStaticMesh
{
	FString PathFileName;			// 원본 파일 경로 (예: "Assets/Models/House.obj")
	TArray<FVertex> Vertices;		// 최종 처리된 정점 (위치, 노멀, UV 결합)
	TArray<uint32> Indices;			// 삼각형 렌더링을 위한 인덱스 버퍼

	FStaticMesh() = default;

	/**
	 * @brief 파일 경로로 생성하는 생성자
	 * @param InPathFileName 원본 메시 파일의 경로
	 */
	FStaticMesh(const FString& InPathFileName)
		: PathFileName(InPathFileName)
	{
	}
};
