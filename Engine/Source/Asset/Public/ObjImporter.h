#pragma once
#include "Asset/Public/StaticMeshData.h"
#include "Global/Types.h"

/**
 * @brief FObjImporter: OBJ 파일의 임포트 및 파싱을 처리
 * @note 언리얼 엔진의 FBX/OBJ 임포터에 해당
 * 원시 OBJ 데이터를 엔진에 바로 사용할 수 있는 형식으로 변환
 */
struct FObjImporter
{
public:
	FObjImporter() = default;
	~FObjImporter() = default;

	/**
	 * @brief OBJ 파일을 임포트하고 원시 객체 데이터로 파싱
	 * @param FilePath 임포트할 OBJ 파일의 경로
	 * @param OutObjectInfos 파싱된 객체 정보 배열
	 * @return 임포트 성공 시 true, 그렇지 않으면 false
	 */
	static bool ImportOBJFile(const FString& FilePath, TArray<FObjInfo>& OutObjectInfos);

	/**
	 * @brief 재질 라이브러리 파일(.mtl)을 파싱
	 * @param MTLFilePath MTL 파일의 경로
	 * @param OutMaterials 파싱된 재질 정보 배열
	 * @return 파싱 성공 시 true, 그렇지 않으면 false
	 */
	static bool ParseMaterialLibrary(const FString& MTLFilePath, TArray<FObjMaterialInfo>& OutMaterials);

	/**
	 * @brief 원시 객체 데이터를 쿠킹된 스태틱 메시 데이터로 변환
	 * @param ObjectInfos 원시 객체 데이터 배열
	 * @param OutStaticMesh 결과로 생성된 쿠킹된 스태틱 메시 데이터
	 * @return 변환 성공 시 true, 그렇지 않으면 false
	 */
	static bool ConvertToStaticMesh(const TArray<FObjInfo>& ObjectInfos, FStaticMesh& OutStaticMesh);

	/**
	 * @brief 전체 임포트 파이프라인: OBJ → 원시 데이터 → 쿠킹된 데이터
	 * @param FilePath 임포트할 OBJ 파일의 경로
	 * @param OutStaticMesh 결과로 생성된 쿠킹된 스태틱 메시 데이터
	 * @return 전체 파이프라인 성공 시 true, 그렇지 않으면 false
	 */
	static bool ImportStaticMesh(const FString& FilePath, FStaticMesh& OutStaticMesh);

private:
	/**
	 * @brief OBJ 파일의 한 줄을 파싱
	 * @param Line 파싱할 줄
	 * @param CurrentObject 현재 파싱 중인 객체
	 * @param AllObjects 파싱 중인 모든 객체 (그룹 관리용)
	 * @param GlobalVertices 전역 정점 목록
	 * @param GlobalUVs 전역 UV 목록
	 * @param GlobalNormals 전역 노멀 목록
	 */
	static void ParseOBJLine(const FString& Line,
		FObjInfo& CurrentObject,
		TArray<FObjInfo>& AllObjects,
		TArray<FVector>& GlobalVertices,
		TArray<FVector2>& GlobalUVs,
		TArray<FVector>& GlobalNormals);

	/**
	 * @brief OBJ 파일에서 면 데이터를 파싱
	 * @param FaceData 면 데이터 문자열 (예: "1/1/1 2/2/2 3/3/3")
	 * @param CurrentObject 면 데이터를 추가할 객체
	 */
	static void ParseFaceData(const FString& FaceData, FObjInfo& CurrentObject);

	/**
	 * @brief 면 인덱스를 적절한 정점 데이터를 가진 삼각형 목록으로 변환
	 * @param ObjectInfo 개별 정점/UV/노멀 배열을 가진 원시 객체 데이터
	 * @param OutVertices 결합된 데이터를 가진 최종 정점 배열
	 * @param OutIndices 삼각형을 위한 최종 인덱스 배열
	 */
	static void ConvertToTriangleList(const FObjInfo& ObjectInfo,
		TArray<FVertex>& OutVertices,
		TArray<uint32>& OutIndices);

	/**
	 * @brief 문자열의 공백을 제거하는 헬퍼 함수
	 * @param Str 공백을 제거할 문자열
	 * @return 공백이 제거된 문자열
	 */
	static FString TrimString(const FString& Str);

	/**
	 * @brief 구분자로 문자열을 분할하는 헬퍼 함수
	 * @param Str 분할할 문자열
	 * @param Delimiter 구분자 문자
	 * @param OutTokens 결과 토큰 배열
	 */
	static void SplitString(const FString& Str, char Delimiter, TArray<FString>& OutTokens);

	/**
	 * @brief OBJ 위치를 UE 좌표계로 변환
	 * @param InVector OBJ 파일의 위치 벡터
	 * @return UE 좌표계의 위치 벡터
	 */
	static FVector PositionToUEBasis(const FVector& InVector);

	/**
	 * @brief OBJ UV 좌표를 UE 좌표계로 변환
	 * @param InVector OBJ 파일의 UV 벡터
	 * @return UE 좌표계의 UV 벡터
	 */
	static FVector2 UVToUEBasis(const FVector2& InVector);
};
