/* compile: gcc -o codepad.exe main.c -lgdi32*/

#include <windows.h>
#include <stdio.h>

#define PROG "Codepad"

#define INITIAL_BUFFER_SIZE 256
#define BUFFER_RESIZE_FACTOR 2

#define MAX_LINES 100
#define LINE_HEIGHT 20

char lines[MAX_LINES][INITIAL_BUFFER_SIZE];
int lineLengths[MAX_LINES];
int totalLines = 0;

char *inputBuffer = NULL;
size_t bufferSize = 0;
size_t bufferPosition = 0;

const char g_szClassName[] = "myWindowClass";

void InitializeBuffer()
{
    bufferSize = INITIAL_BUFFER_SIZE;
    inputBuffer = (char *)malloc(bufferSize);
    if (!inputBuffer) {
        // Handle memory allocation failure
        exit(EXIT_FAILURE);
    }

    memset(inputBuffer, 0, bufferSize);
}

void AppendCharacterToBuffer(char ch)
{
    if (bufferPosition >= bufferSize - 1) {
        // Resize the buffer
        size_t newBufferSize = bufferSize * BUFFER_RESIZE_FACTOR;
        char *newBuffer = (char *)malloc(newBufferSize);
        if (!newBuffer) {
            // Handle memory allocation failure
            exit(EXIT_FAILURE);
        }

        // Copy the old contents and zero out the new space
        memcpy(newBuffer, inputBuffer, bufferSize);
        memset(newBuffer + bufferSize, 0, newBufferSize - bufferSize);

        free(inputBuffer);
        inputBuffer = newBuffer;
        bufferSize = newBufferSize;
    }
    inputBuffer[bufferPosition++] = ch;
    inputBuffer[bufferPosition] = '\0';
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int currentLine = 0;
    static int scrollPosition = 0;
    const int lineHeight = 20;

    switch(msg)
    {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            free(inputBuffer);
            PostQuitMessage(0);
            break;
        case WM_KEYDOWN:
            if (wParam == 'W' && GetKeyState(VK_CONTROL) < 0) {
                // Ctrl+W was pressed
                PostQuitMessage(0);
            } else if (wParam == VK_BACK) {
                // Backspace was pressed
                if (bufferPosition > 0) {
                    bufferPosition--;
                    inputBuffer[bufferPosition] = '\0';
                    InvalidateRect(hwnd, NULL, TRUE);
                } else if (totalLines > 0) {
                    totalLines--;
                    bufferPosition = lineLengths[totalLines];
                    strcpy(inputBuffer, lines[totalLines]);
                    InvalidateRect(hwnd, NULL, TRUE);
                    if (totalLines == MAX_LINES-1) scrollPosition -= 10;
                    if (scrollPosition > 0 && totalLines % 2 == 1) {
                        scrollPosition--;
                    }
                }
            } else if (wParam == VK_RETURN) {
                if (totalLines < MAX_LINES) {
                    strcpy(lines[totalLines], inputBuffer);
                    lineLengths[totalLines] = bufferPosition;
                    totalLines++;
                    bufferPosition = 0;
                    inputBuffer[bufferPosition] = '\0';
                    InvalidateRect(hwnd, NULL, TRUE);
                }
                if (totalLines > 20 && totalLines % 2 == 0) {
                    scrollPosition++;
                }
            }
            break;
        case WM_CHAR:
            if (wParam >= 32 && wParam <= 126) {
                char ch = (char)wParam;
                AppendCharacterToBuffer(ch);
                InvalidateRect(hwnd, NULL, TRUE);
                printf("Character input: %c\n", ch);
            }
            break;
        case WM_MOUSEWHEEL:
            {
                int delta = GET_WHEEL_DELTA_WPARAM(wParam);
                scrollPosition -= delta / WHEEL_DELTA;
                if (scrollPosition < 0) {
                    scrollPosition = 0;
                }
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);

                int y = 10 - scrollPosition * LINE_HEIGHT; // Adjust y-coordinate based on scroll position
                int linesToRender = min(totalLines, scrollPosition + MAX_LINES); // Calculate lines to render

                for (int i = scrollPosition; i < linesToRender; i++) {
                    // Set text color to grey for line numbers
                    SetTextColor(hdc, RGB(128, 128, 128));

                    // Print line number
                    char lineNumber[10];
                    snprintf(lineNumber, sizeof(lineNumber), "%d: ", i + 1);
                    TextOut(hdc, 10, y, lineNumber, strlen(lineNumber));

                    // Reset text color
                    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));

                    // Print line content immediately after line numbers
                    TextOut(hdc, 10 + 30, y, lines[i], lineLengths[i]);
                    y += LINE_HEIGHT; // Move to the next line
                }

                // Display the current line being edited
                if (inputBuffer) {
                    char lineNumber[10];
                    snprintf(lineNumber, sizeof(lineNumber), "%d: ", totalLines + 1);
                    TextOut(hdc, 10, y, lineNumber, strlen(lineNumber));
                    TextOut(hdc, 10 + 30, y, inputBuffer, bufferPosition);
                }

                EndPaint(hwnd, &ps);
            }
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    InitializeBuffer();

    //Step 1: Registering the Window Class
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "Window Registration Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Step 2: Creating the Window
    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, g_szClassName, PROG, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 600, 800, NULL, NULL, hInstance, NULL);

    if(hwnd == NULL)
    {
        MessageBox(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Step 3: The Message Loop
    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}