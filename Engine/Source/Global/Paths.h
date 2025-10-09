#pragma once

class FPaths
{
public:
	/**
	 * @brief 프로젝트의 루트 디렉토리 경로를 반환합니다.
	 * @return 프로젝트 루트 디렉토리 경로
	 */
	static const std::filesystem::path& GetProjectRootDir();

	/**
	 * @brief 셰이더 파일이 저장된 디렉토리 경로를 반환합니다.
	 * @return 셰이더 디렉토리 경로
	 */
	static std::filesystem::path GetShaderPath();

	/**
	 * @brief 콘텐츠(에셋) 파일이 저장된 디렉토리 경로를 반환합니다.
	 * @return 콘텐츠 디렉토리 경로
	 */
	static std::filesystem::path GetContentPath();

	/**
	 * @brief 주어진 경로를 프로젝트 루트 기준으로 변환합니다.
	 * @param InPath 변환할 상대 경로
	 * @return 전체 절대 경로
	 */
	static std::filesystem::path ConvertToProjectAbsolutePath(const std::filesystem::path& InPath);

private:
	// 프로젝트 루트 디렉토리를 저장하는 정적 변수
	static std::filesystem::path ProjectRootDir;
};