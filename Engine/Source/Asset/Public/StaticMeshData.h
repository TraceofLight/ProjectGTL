#pragma once
#include "Global/Types.h"
#include "Global/Vector.h"
#include "Global/CoreTypes.h"

struct FStaticMeshSection;
class UMaterialInterface;

/**
 * @brief OBJ 파일의 머티리얼 정보 (통합 버전)
 * MTL 파일에서 읽어온 머티리얼 속성들 + DrawIndexedPrimitivesCommand 호환성
 */
struct FObjMaterialInfo
{
	// 기본 머티리얼 정보
	FString MaterialName;
	
	// 텍스쳐 경로들
	FString DiffuseTexturePath;
	FString NormalTexturePath;
	FString SpecularTexturePath;
	
	// 색상 속성들 (기존 버전 유지)
	FVector AmbientColorScalar = FVector(0.2f, 0.2f, 0.2f);
	FVector DiffuseColorScalar = FVector(1.0f, 1.0f, 1.0f); 
	FVector SpecularColorScalar = FVector(1.0f, 1.0f, 1.0f);
	float ShininessScalar = 32.0f;
	float TransparencyScalar = 1.0f;
	
	// DrawIndexedPrimitivesCommand 호환성을 위한 추가 속성들
	FVector DiffuseColor = FVector(0.8f, 0.8f, 0.8f);      // Kd
	FVector AmbientColor = FVector(0.2f, 0.2f, 0.2f);      // Ka
	FVector SpecularColor = FVector(1.0f, 1.0f, 1.0f);     // Ks
	FVector EmissiveColor = FVector(0.0f, 0.0f, 0.0f);     // Ke
	float SpecularExponent = 32.0f;                        // Ns
	float OpticalDensity = 1.0f;                           // Ni
	float Transparency = 0.0f;                             // Tr or d
	uint32 IlluminationModel = 2;                          // illum
	
	// 텍스쳐 파일명 (DrawIndexedPrimitivesCommand에서 사용)
	std::string DiffuseTextureFileName;     // map_Kd

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
 * @brief 스태틱 메시 내 객체의 섹션 정보
 * 각 섹션은 동일한 머티리얼을 사용하는 삼각형 그룹을 나타냄.
 */
struct FObjSectionInfo
{
	FString MaterialName;	// 섹션에서 사용할 머티리얼 이름
	int32 StartIndex = 0;	// 섹션 시작 인덱스 (파싱된 인덱스 배열 기준)
	int32 IndexCount = 0;	// 섹션에 포함된 인덱스 수

	FObjSectionInfo() = default;

	FObjSectionInfo(const FString& InMaterialName, int32 InStartIndex)
		: MaterialName(InMaterialName)
		, StartIndex(InStartIndex)
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
	TArray<uint32> NormalIndexList;			// OBJ 'f' 명령어의 노멀 인덱스 (NormalList 참조)
	TArray<FObjSectionInfo> Sections;		// OBJ 'usemtl' 명령어에 따라 분리된 동일 머티리얼을 사용하는 섹션 정보
	TMap<FString, FObjMaterialInfo> MaterialInfos;	// 참조하는 머티리얼 정보. Sections에서 MaterialName을 사용해 MaterialInfos 찾음.

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
 * @brief 언리얼 엔진 스타일 스테틱 메시 섹션 구조
 * @note 같은 섹션 내의 triangle들은 같은 material을 사용
 * 기존 FGroupInfo/FMaterialSlot를 대체하는 언리얼 스타일 구조
 */
struct FStaticMeshSection
{
	// 기본 인덱스 정보
	int32 StartIndex = 0;   // 섹션의 시작 인덱스 (Indices 배열에서)
	int32 IndexCount = 0;  // 섹션의 인덱스 개수
	int32 MaterialSlotIndex = -1; // 슬롯은 UStaticMesh에서 관리
	
	// 임시 데이터: OBJ 파싱 단계에서만 사용, 런타임에서는 MaterialSlotIndex 사용
	FString MaterialName; // OBJ 'usemtl' 명령어에서 가져온 원본 이름 (파싱 전용)

	// DrawIndexedPrimitivesCommand 호환성을 위한 헬퍼 메서드들
	FString GroupName;       // 섹션 그룹 이름 (디버깅용)
	uint32 GetStartIndex() const { return static_cast<uint32>(StartIndex); }
	uint32 GetIndexCount() const { return static_cast<uint32>(IndexCount); }
	
	// Material Slot 매핑 로직
	bool HasValidMaterialSlot() const { return MaterialSlotIndex >= 0; }
	
	// 파싱 단계 전용 (런타임에서는 MaterialSlotIndex 사용 권장)
	bool HasMaterialName() const { return !MaterialName.IsEmpty(); }
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
	TArray<FStaticMeshSection> Sections; // 다중 머터리얼 사용을 위한 메시 섹션

	FStaticMesh()
	{
		UE_LOG_DEBUG("FStaticMesh: Default constructor called");
	}

	// 복사 생성자
	FStaticMesh(const FStaticMesh& Other)
		: PathFileName(Other.PathFileName)
		, Vertices(Other.Vertices)
		, Indices(Other.Indices)
		, Sections(Other.Sections)
	{
		UE_LOG_DEBUG("FStaticMesh: Copy constructor called for %s", PathFileName.c_str());
	}

	// 복사 생성 연산자
	FStaticMesh& operator=(const FStaticMesh& Other)
	{
		if (this != &Other)
		{
			PathFileName = Other.PathFileName;
			Vertices = Other.Vertices;
			Indices = Other.Indices;
			Sections = Other.Sections;
			UE_LOG_DEBUG("FStaticMesh: Copy assignment operator called for %s", PathFileName.c_str());
		}
		return *this;
	}

	/**
	 * @brief 파일 경로로 생성하는 생성자
	 * @param InPathFileName 원본 메시 파일의 경로
	 */
	FStaticMesh(const FString& InPathFileName)
		: PathFileName(InPathFileName)
	{
	}
};
