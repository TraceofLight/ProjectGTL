#pragma once
#include <chrono>

using std::chrono::high_resolution_clock;

class FAppWindow;

extern float GDeltaTime;

/**
 * @brief Main Client Class
 * Engine entry point, Manage overall execution flow
 * @var AcceleratorTable 키보드 단축키 테이블 핸들
 * @var MainMessage 윈도우 메시지 구조체
 * @var Window 윈도우 객체 포인터
 */
class FEngineLoop
{
public:
    int Run(HINSTANCE InInstanceHandle, int InCmdShow);

    // Special member function
    FEngineLoop();
    ~FEngineLoop();

private:
	HACCEL AcceleratorTable = nullptr;
	MSG MainMessage = {};
	FAppWindow* Window = nullptr;

	high_resolution_clock::time_point PreviousTime;
	high_resolution_clock::time_point CurrentTime;

	void PreInit(HINSTANCE InInstanceHandle, int InCmdShow);
    void Init() const;
    void Tick();
    void MainLoop();
	void Exit() const;

	void UpdateDeltaTime();
};
