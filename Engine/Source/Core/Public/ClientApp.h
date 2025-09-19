#pragma once

//class UEditor;
class FAppWindow;

/**
 * @brief Main Client Class
 * Application Entry Point, Manage Overall Execution Flow
 *
 * @var AcceleratorTable 키보드 단축키 테이블 핸들
 * @var MainMessage 윈도우 메시지 구조체
 * @var Window 윈도우 객체 포인터
 */
class FEngineLoop
{
public:
    int Run(HINSTANCE InInstanceHandle, int InCmdShow);

    // Special Member Function
    FEngineLoop();
    ~FEngineLoop();

private:
	void PreInit(HINSTANCE InInstanceHandle, int InCmdShow);
    void Init() const;
    void Tick() const;
    void MainLoop();
	void Exit() const;

    HACCEL AcceleratorTable;
    MSG MainMessage;
    FAppWindow* Window;
};
