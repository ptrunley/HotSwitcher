// util.cpp
//
// Various utility routines

#include "stdafx.h"

extern BOOL g_fDebug;

VOID DebugOut(
    __in const WCHAR *pwszFormat,
    ...
)
{
    va_list      args;

    if( g_fDebug )
    {
        va_start( args, pwszFormat );
        vfwprintf( stderr, pwszFormat, args );
        va_end( args );
    }
}

HRESULT GetLastFailureAsHRESULT()
{
    HRESULT             hr          = S_OK;
    DWORD               dwResult    = ERROR_SUCCESS;
    
    dwResult = ::GetLastError();
    
    hr = HRESULT_FROM_WIN32( dwResult );
    if ( SUCCEEDED( hr ) )
    {
        hr = E_FAIL;
    }

    return hr;
}

BOOL 
GetProcessImageName(
    __in HWND hwnd,
    __out_ecount(cchImage) WCHAR *pwszImage,
    __in DWORD cchImage
)
{
    BOOL fResult = FALSE;
    HANDLE hProcess = NULL;
    DWORD dwProcessId = 0;
    DWORD dwThreadId = 0;
    DWORD cchTemp = 0;

    dwThreadId = GetWindowThreadProcessId( hwnd, &dwProcessId );

    hProcess = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcessId );
    if( hProcess == NULL )
    {
        DebugOut( L"Failed OpenProcess for hwnd %p with %d\n", hwnd, GetLastError() );
        goto Exit;
    }

    cchTemp = cchImage;
    if( !QueryFullProcessImageNameW( hProcess, 0, pwszImage, &cchTemp ) )
    {
        DebugOut( L"Failed QueryFullProcessImageNameW for hwnd %p with %d\n", hwnd, GetLastError() );
        goto Exit;
    }

    if( cchTemp >= (cchImage - 1) )
    {
        goto Exit;
    }

    pwszImage[cchTemp] = 0;

    fResult = TRUE;

Exit:

    if( hProcess != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hProcess );
        hProcess = INVALID_HANDLE_VALUE;
    }

    return fResult;
}
