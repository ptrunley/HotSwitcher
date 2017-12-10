//
// Main.cpp: routines for the general test harness.
//
// All real test code is in test.cpp.
//

#include "stdafx.h"

BOOL g_fUsage = FALSE;
BOOL g_fDebug = FALSE;
BOOL g_fListWindows = FALSE;
BOOL g_fListKeys = FALSE;

void
Usage(
    const WCHAR *program
)
{
    fwprintf( stderr, L"Use hotkeys to switch windows\n\n" );
    fwprintf( stderr, L"%s [/d] [/l]\n\n", program );
    fwprintf( stderr, L"  /d            Debug mode\n" );
    fwprintf( stderr, L"  /k            Display help for key names\n" );
    fwprintf( stderr, L"  /l            List window titles\n" );
    fwprintf( stderr, L"\n" );
}

int
ParseSwitches(
    int argc,
    WCHAR **argv
)
{
    int startargs = argc;
    WCHAR *ta = NULL;

    argc--; argv++;

    while (argc)
    {
        ta = *argv;
        if ((*ta == L'-') || (*ta == L'/'))
        {
            ta++;
            while (*ta)
            {
                switch (*ta)
                {
                case L'?':
                    g_fUsage = TRUE;
                    break;

                case L'd':
                case L'D':
                    g_fDebug = TRUE;
                    break;

                case L'k':
                case L'K':
                    g_fListKeys = TRUE;
                    break;

                case L'l':
                case L'L':
                    g_fListWindows = TRUE;
                    break;

                default:
                    fwprintf( stderr, L"Unknown flag %s\n", *argv);
                    g_fUsage = TRUE;
                    goto done;
                }
                ta++;
            }
        } else {
            break;
        }

        argc--; argv++;
    }

done:

    return startargs - argc;
}

BOOL CALLBACK
ListWindowsEnumCallback(
    __in HWND hwnd,
    __in LPARAM lParam
)
{
    WCHAR  wszText[1024] = L"";
    WINDOWINFO info = {};

    info.cbSize = sizeof info;
    if( !GetWindowInfo( hwnd, &info ) )
    {
        fwprintf( stderr, L"GetWindowInfo for %p failed with %d\n", hwnd, GetLastError() );
        goto Exit;
    }

    if( ( info.dwStyle & VALID_WINDOW_STYLE_MASK ) != VALID_WINDOW_STYLE_VALUE )
    {
        DebugOut(L"Skipping: dwStyle=0x%08x\n", info.dwStyle );
        goto Exit;
    }
    
    if( GetWindowTextW( hwnd, wszText, ARRAYSIZE( wszText ) ) != 0 )
    {
        wprintf( L"%s\n", wszText );
    }

    if( GetProcessImageName( hwnd, wszText, ARRAYSIZE( wszText ) ) )
    {
        wprintf( L"  %s\n", wszText );
        goto Exit;
    }


Exit:

    return TRUE;
}

BOOL
ListWindows(
)
{
    BOOL fResult = FALSE;

    if( !EnumWindows( ListWindowsEnumCallback, 0 ) )
    {
        goto Exit;
    }

    fResult = TRUE;

Exit:

    return fResult;
}

BOOL
RunSwitcher(
)
{
    BOOL fResult = FALSE;
    CSwitcher *pSwitcher = NULL;
    MSG msg = {0};

    pSwitcher = new CSwitcher();
    if( pSwitcher == NULL )
    {
        goto Exit;
    }

    if( !pSwitcher->LoadFromRegistry() )
    {
        goto Exit;
    }

    if( !pSwitcher->RegisterKeys( NULL ) )
    {
        goto Exit;
    }

    if( !g_fDebug )
    {
        FreeConsole();
    }

    while( GetMessage( &msg, NULL, 0, 0 ) != 0 )
    {
        if( msg.message == WM_HOTKEY )
        {
            DebugOut( L"WM_HOTKEY received (wParam=%p, lParam=%p)\n", msg.wParam, msg.lParam );
            pSwitcher->ProcessHotKeyMessage( &msg );
        }
    }

    fResult = TRUE;

Exit:

    delete pSwitcher;

    return fResult;
}


int __cdecl
wmain(
    int argc,
    WCHAR **argv
)
{
    int iRet = -1;
    int iNext = 0;
    
    iNext = ParseSwitches( argc, argv );

    if( g_fUsage )
    {
        Usage( argv[0] );
        goto Exit;
    }

    if( g_fListWindows )
    {
        if( !ListWindows() )
        {
            goto Exit;
        }
    }
    else if( g_fListKeys )
    {
        ShowKeys();
    }
    else
    {
        if( !RunSwitcher() )
        {
            goto Exit;
        }
    }

    iRet = 0;

Exit:
    
    return iRet;
}

