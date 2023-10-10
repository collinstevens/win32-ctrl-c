#include <windows.h>

#include <io.h>
#include <stdio.h>
#include <ios>

static bool Running = true;

void RouteStdioToConsole(bool create_console_if_not_found) {
  // Don't change anything if stdout or stderr already point to a
  // valid stream.
  //
  // If we are running under Buildbot or under Cygwin's default
  // terminal (mintty), stderr and stderr will be pipe handles.  In
  // that case, we don't want to open CONOUT$, because its output
  // likely does not go anywhere.
  //
  // We don't use GetStdHandle() to check stdout/stderr here because
  // it can return dangling IDs of handles that were never inherited
  // by this process.  These IDs could have been reused by the time
  // this function is called.  The CRT checks the validity of
  // stdout/stderr on startup (before the handle IDs can be reused).
  // _fileno(stdout) will return -2 (_NO_CONSOLE_FILENO) if stdout was
  // invalid.
  if (_fileno(stdout) >= 0 || _fileno(stderr) >= 0) {
    MessageBox(NULL, TEXT("CONSOLE EXISTS"), TEXT("AttachConsole"), MB_OK);
    // _fileno was broken for SUBSYSTEM:WINDOWS from VS2010 to VS2012/2013.
    // http://crbug.com/358267. Confirm that the underlying HANDLE is valid
    // before aborting.

    intptr_t stdout_handle = _get_osfhandle(_fileno(stdout));
    intptr_t stderr_handle = _get_osfhandle(_fileno(stderr));
    if (stdout_handle >= 0 || stderr_handle >= 0)
      return;
  }

  if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
    unsigned int result = GetLastError();
    // Was probably already attached.
    if (result == ERROR_ACCESS_DENIED)
        return;

    // Don't bother creating a new console for each child process if the
    // parent process is invalid (eg: crashed).
    if (result == ERROR_GEN_FAILURE)
        return;

    if (create_console_if_not_found) {
      // Make a new console if attaching to parent fails with any other error.
      // It should be ERROR_INVALID_HANDLE at this point, which means the
      // browser was likely not started from a console.
      AllocConsole();
    } else {
      return;
    }
  }

  // Arbitrary byte count to use when buffering output lines.  More
  // means potential waste, less means more risk of interleaved
  // log-lines in output.
  enum { kOutputBufferSize = 64 * 1024 };

  if (freopen("CONOUT$", "w", stdout)) {
    setvbuf(stdout, nullptr, _IONBF, kOutputBufferSize);
    // Overwrite FD 1 for the benefit of any code that uses this FD
    // directly.  This is safe because the CRT allocates FDs 0, 1 and
    // 2 at startup even if they don't have valid underlying Windows
    // handles.  This means we won't be overwriting an FD created by
    // _open() after startup.
    _dup2(_fileno(stdout), 1);
  }
  if (freopen("CONOUT$", "w", stderr)) {
    setvbuf(stderr, nullptr, _IONBF, kOutputBufferSize);
    _dup2(_fileno(stderr), 2);
  }

  // Fix all cout, wcout, cin, wcin, cerr, wcerr, clog and wclog.
  std::ios_base::sync_with_stdio();
}

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
        case CTRL_C_EVENT:
            printf("Caught CTRL_C_EVENT, ignoring\r\n");
            //Running = false;
            return TRUE;

        case CTRL_BREAK_EVENT:
            printf("Caught CTRL_BREAK_EVENT, quitting\r\n");
            Running = false;
            return TRUE;

        default:
            return FALSE;
    }
}

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT Result = 0;

    switch (uMsg)
    {
        case WM_SIZE:
        {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            printf("WM_SIZE %u %u\r\n", width, height);
            return 0;
        }
        break;

        case WM_CLOSE:
        {
            printf("WM_CLOSE\r\n");
            Running = false;
        }
        break;

        case WM_DESTROY:
        {
            printf("WM_DESTROY\r\n");
            Running = false;
        }
        break;

        case WM_QUERYENDSESSION:
        {
            printf("WM_QUERYENDSESSION\r\n");
            Running = false;
            // Check `lParam` for which system shutdown function and handle events.
            // See https://learn.microsoft.com/windows/win32/shutdown/wm-queryendsession
            return TRUE; // Respect user's intent and allow shutdown.
        }
        break;

        case WM_ENDSESSION:
        {
            printf("WM_ENDSESSION\r\n");
            Running = false;
            // Check `lParam` for which system shutdown function and handle events.
            // See https://learn.microsoft.com/windows/win32/shutdown/wm-endsession
            return 0; // We have handled this message.
        }
        break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PSTR pCmdLine,
    int nCmdShow)
{
    RouteStdioToConsole(false);

    HANDLE hOut = (HANDLE)_get_osfhandle(_fileno(stdout));
    if (hOut == INVALID_HANDLE_VALUE) 
    {
        MessageBox(NULL, TEXT("GetStdHandle"), TEXT("Console Error"), MB_OK);
        return GetLastError();
    }

    printf("Redirected STDIO\r\n");

    WNDCLASS wc = {};
    wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("Win32CtrlC");

    RegisterClass(&wc);
    
    HWND hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        TEXT("Win32 CTRL-C"),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL);

    ShowWindow(hwnd, nCmdShow);
    printf("Displayed Win32 GUI\r\n");

    if (SetConsoleCtrlHandler(CtrlHandler, TRUE))
    {
        printf("Assigned SetConsoleCtrlHandler callback\r\n");

        while (Running)
        {
            MSG uMsg;
            while (PeekMessage(&uMsg, 0, 0, 0, PM_REMOVE))
            {
                if (uMsg.message == WM_QUIT)
                    Running = false;

                TranslateMessage(&uMsg);
                DispatchMessage(&uMsg);
            }
        }
    }
    else
    {
        printf("ERROR: Could not set control handler\r\n");
        return 1;
    }

    printf("Exiting...\r\n");
    FreeConsole();

    return 0;
}