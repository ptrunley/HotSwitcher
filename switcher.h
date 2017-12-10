#pragma once

#define VALID_WINDOW_STYLE_MASK   (WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_MINIMIZE)
#define VALID_WINDOW_STYLE_VALUE  (WS_CAPTION | WS_SYSMENU | WS_VISIBLE)

typedef std::basic_regex<WCHAR> RegExp;

class CHotKey
{
public:

    CHotKey();

    ~CHotKey();

    BOOL Activate(
        __in BOOL fNextWindow
    );
    
    BOOL Initialize(
        __in const WCHAR *pwszKey,
        __in const WCHAR *pwszExpr
    );

    DWORD       m_dwMod;
    DWORD       m_dwKey;

private:

    BOOL ParseModifier(
        __inout const WCHAR **ppwszKey,
        __inout DWORD *pdwMod
    );

    BOOL ParseKey(
        __in const WCHAR *pwszKey,
        __out DWORD *pdwKey
    );

    BOOL FindMatches(
        __out_ecount(dwMatches) HWND *phMatches,
        __in DWORD dwMatches,
        __out DWORD *pdwActual
    );

    RegExp      *m_pExpr;
    DWORD       m_dwLastMatch;
    HWND        m_hLastMatch;
};

class CSwitcher
{
public:

    CSwitcher();

    ~CSwitcher();

    BOOL LoadFromRegistry();

    BOOL RegisterKeys(
        __in HWND hWnd
    );

    BOOL ProcessHotKeyMessage(
        __in MSG  *pMsg
    );

private:

    VOID FreeKeys();

    CHotKey      *m_rgpKeys[100];
    DWORD        m_dwKeys;
    DWORD        m_dwLastKey;
    HWND         m_hWnd;
};

VOID ShowKeys();


    


    
