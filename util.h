#pragma once

VOID DebugOut(__in const WCHAR *pwszFormat, ...);

HRESULT GetLastFailureAsHRESULT();

BOOL GetProcessImageName(__in HWND hwnd,
                         __out_ecount(cchImage) WCHAR *pwszImage,
                         __in DWORD cchImage);


