#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <string>

#define TARGET_RADIUS 20
#define TIMER_ID 1
#define MAX_TARGETS 25

enum GameState {
    MENU,
    STATIC,
    STRAFE
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void DrawTarget(HDC hdc, int x, int y);
void ResetGame();
void DrawBackground(HDC hdc);
void StartGame(GameState state);
void UpdateStrafeTargets();

int targetX, targetY;
int strafeTargetX[3], strafeTargetY[3];
int score = 0;
int targetsShot = 0;
DWORD startTime = 0;
DWORD bestTime = 0;
GameState gameState = MENU;
BOOL strafeDirection[3] = { TRUE, TRUE, TRUE };

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"AimTrainerClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_CROSS);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Aim Trainer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            srand((unsigned int)time(NULL));
            SetTimer(hwnd, TIMER_ID, 10, NULL);
            return 0;
        }
        case WM_TIMER: {
            if (wParam == TIMER_ID) {
                if (gameState == STRAFE) {
                    UpdateStrafeTargets();
                    InvalidateRect(hwnd, NULL, TRUE);
                }
            }
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Piesia backgrounda
            DrawBackground(hdc);

            // Nupiesia priesus
            if (gameState == STATIC && targetsShot < MAX_TARGETS) {
                DrawTarget(hdc, targetX, targetY);
            } else if (gameState == STRAFE) {
                for (int i = 0; i < 3; ++i) {
                    DrawTarget(hdc, strafeTargetX[i], strafeTargetY[i]);
                }
            }

            // Taskai ir taimeris
            WCHAR scoreText[50];
            wsprintfW(scoreText, L"Score: %d", score);
            TextOutW(hdc, 10, 10, scoreText, lstrlenW(scoreText));

            if (startTime > 0) {
                DWORD elapsedTime = GetTickCount() - startTime;
                WCHAR timerText[50];
                wsprintfW(timerText, L"Time: %d.%03d seconds", elapsedTime / 1000, elapsedTime % 1000);
                TextOutW(hdc, 10, 30, timerText, lstrlenW(timerText));
            }

            if (bestTime > 0) {
                WCHAR bestTimeText[50];
                wsprintfW(bestTimeText, L"Best Time: %d.%03d seconds", bestTime / 1000, bestTime % 1000);
                TextOutW(hdc, 10, 50, bestTimeText, lstrlenW(bestTimeText));
            }

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            if (gameState == MENU) {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                if (x > 100 && x < 300 && y > 100 && y < 150) {
                    StartGame(STATIC);
                } else if (x > 100 && x < 300 && y > 200 && y < 250) {
                    StartGame(STRAFE);
                }
                return 0;
            }

            if (targetsShot >= MAX_TARGETS) {
                return 0;
            }

            if (startTime == 0) {
                startTime = GetTickCount();
            }

            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            if (gameState == STATIC) {
                int dx = x - targetX;
                int dy = y - targetY;
                if (dx * dx + dy * dy <= TARGET_RADIUS * TARGET_RADIUS) {
                    score++;
                    targetsShot++;
                    if (targetsShot >= MAX_TARGETS) {
                        DWORD elapsedTime = GetTickCount() - startTime;
                        if (bestTime == 0 || elapsedTime < bestTime) {
                            bestTime = elapsedTime;
                        }
                        KillTimer(hwnd, TIMER_ID);
                        MessageBoxW(hwnd, L"Congratulations! You've completed the game.", L"Game Over", MB_OK);
                        ResetGame();
                    } else {
                        RECT rect;
                        GetClientRect(hwnd, &rect);
                        targetX = rand() % (rect.right - 2 * TARGET_RADIUS) + TARGET_RADIUS;
                        targetY = rand() % (rect.bottom - 2 * TARGET_RADIUS) + TARGET_RADIUS;
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                }
            } else if (gameState == STRAFE) {
                for (int i = 0; i < 3; ++i) {
                    int dx = x - strafeTargetX[i];
                    int dy = y - strafeTargetY[i];
                    if (dx * dx + dy * dy <= TARGET_RADIUS * TARGET_RADIUS) {
                        score++;
                        RECT rect;
                        GetClientRect(hwnd, &rect);
                        strafeTargetX[i] = rand() % (rect.right - 2 * TARGET_RADIUS) + TARGET_RADIUS;
                        strafeTargetY[i] = rand() % (rect.bottom - 2 * TARGET_RADIUS) + TARGET_RADIUS;
                        break;
                    }
                }
                InvalidateRect(hwnd, NULL, TRUE);
            }

            return 0;
        }
        case WM_DESTROY: {
            KillTimer(hwnd, TIMER_ID);
            PostQuitMessage(0);
            return 0;
        }
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

void DrawTarget(HDC hdc, int x, int y) {
    HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
    Ellipse(hdc, x - TARGET_RADIUS, y - TARGET_RADIUS, x + TARGET_RADIUS, y + TARGET_RADIUS);
    SelectObject(hdc, oldBrush);
    DeleteObject(brush);
}

void ResetGame() {
    score = 0;
    targetsShot = 0;
    startTime = 0;
    gameState = MENU;
    InvalidateRect(NULL, NULL, TRUE);
}

void DrawBackground(HDC hdc) {
    RECT rect;
    GetClientRect(GetForegroundWindow(), &rect);
    HBRUSH backgroundBrush = CreateSolidBrush(RGB(240, 240, 240));
    FillRect(hdc, &rect, backgroundBrush);
    DeleteObject(backgroundBrush);

    // backgroundo piesimas - liniju
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    for (int y = 0; y < rect.bottom; y += 20) {
        MoveToEx(hdc, 0, y, NULL);
        LineTo(hdc, rect.right, y);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void StartGame(GameState state) {
    gameState = state;
    score = 0;
    targetsShot = 0;
    startTime = 0;

    RECT rect;
    GetClientRect(GetForegroundWindow(), &rect);
    if (state == STATIC) {
        targetX = rand() % (rect.right - 2 * TARGET_RADIUS) + TARGET_RADIUS;
        targetY = rand() % (rect.bottom - 2 * TARGET_RADIUS) + TARGET_RADIUS;
    } else if (state == STRAFE) {
        for (int i = 0; i < 3; ++i) {
            strafeTargetX[i] = rand() % (rect.right - 2 * TARGET_RADIUS) + TARGET_RADIUS;
            strafeTargetY[i] = rand() % (rect.bottom - 2 * TARGET_RADIUS) + TARGET_RADIUS;
            strafeDirection[i] = rand() % 2;
        }
    }
    InvalidateRect(NULL, NULL, TRUE);
}

void UpdateStrafeTargets() {
    RECT rect;
    GetClientRect(GetForegroundWindow(), &rect);
    for (int i = 0; i < 3; ++i) {
        if (strafeDirection[i]) {
            strafeTargetX[i] += 2;
            if (strafeTargetX[i] + TARGET_RADIUS > rect.right) {
                strafeDirection[i] = FALSE;
            }
        } else {
            strafeTargetX[i] -= 2;
            if (strafeTargetX[i] - TARGET_RADIUS < 0) {
                strafeDirection[i] = TRUE;
            }
        }
        
        if (i == 1) {
            if (strafeDirection[i]) {
                strafeTargetY[i] += 2;
                if (strafeTargetY[i] + TARGET_RADIUS > rect.bottom) {
                    strafeDirection[i] = FALSE;
                }
            } else {
                strafeTargetY[i] -= 2;
                if (strafeTargetY[i] - TARGET_RADIUS < 0) {
                    strafeDirection[i] = TRUE;
                }
            }
        }
    }
}
