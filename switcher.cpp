#include "stdafx.h"

#include "keys.h"

CHotKey::CHotKey() :
    m_dwMod( 0 ),
    m_dwKey( 0 ),
    m_pExpr( NULL ),
    m_dwLastMatch( DWORD_MAX ),
    m_hLastMatch( NULL )
{
}

CHotKey::~CHotKey()
{
    delete m_pExpr;
    m_pExpr = NULL;
}

BOOL
CHotKey::ParseModifier(
    __inout const WCHAR **ppwszKey,
    __inout DWORD *pdwMod
)
{
    DWORD i = 0;
    DWORD dwLen = 0;

    for( i = 0; i < ARRAYSIZE( ModKeys ); ++i )
    {
        dwLen = (DWORD) wcslen( ModKeys[i].pwszName );
        if( _wcsnicmp( *ppwszKey, ModKeys[i].pwszName, dwLen ) == 0 && (*ppwszKey)[dwLen] == L'+' )
        {
            *pdwMod |= ModKeys[i].dwValue;
            (*ppwszKey) += (dwLen + 1);
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
CHotKey::ParseKey(
    __in const WCHAR *pwszKey,
    __out DWORD *pdwKey
)
{
    DWORD i = 0;

    for( i = 0; i < ARRAYSIZE( Keys ); ++i )
    {
        if( _wcsicmp( pwszKey, Keys[i].pwszName ) == 0 )
        {
            *pdwKey = Keys[i].dwValue;
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
CHotKey::Initialize(
    __in const WCHAR *pwszKey,
    __in const WCHAR *pwszExpr
)
{
    BOOL      fResult = FALSE;
    DWORD     dwMod = 0;
    DWORD     dwKey = 0;
    RegExp    *pExpr = NULL;

    while( ParseModifier( &pwszKey, &dwMod ) )
    {
    }

    if( !ParseKey( pwszKey, &dwKey ) )
    {
        goto Exit;
    }

    try
    {
        pExpr = new RegExp( pwszExpr, std::regex::ECMAScript | std::regex::icase );
    }
    catch( const std::regex_error& )
    {
    }
    if( pExpr == NULL )
    {
        goto Exit;
    }

    m_dwMod = dwMod;
    m_dwKey = dwKey;
    m_pExpr = pExpr;
    pExpr = NULL;

    fResult = TRUE;

Exit:

    delete pExpr;
    pExpr = NULL;

    return fResult;
}

BOOL
CHotKey::Activate(
        __in BOOL fNextWindow
    )
{
    BOOL fResult = FALSE;
    HWND hForeground = NULL;
    HWND phMatches[20] = {};
    DWORD dwMatches = 0;

    if( !FindMatches( phMatches, ARRAYSIZE( phMatches ), &dwMatches ) )
    {
        goto Exit;
    }

    if( dwMatches == 0 )
    {
        fResult = TRUE;
        goto Exit;
    }

    hForeground = GetForegroundWindow();
    if( m_hLastMatch == hForeground && fNextWindow )
    {
        ++m_dwLastMatch;
    }
    if( m_dwLastMatch >= dwMatches )
    {
        m_dwLastMatch = 0;
    }
    m_hLastMatch = phMatches[m_dwLastMatch];

    if( !SetForegroundWindow( m_hLastMatch ) )
    {
        goto Exit;
    }

    fResult = TRUE;

Exit:

    return fResult;
}

struct FindMatchesEnumContext
{
    RegExp         *pExpr;
    HWND           *phMatches;
    DWORD          dwMaxMatches;
    DWORD          dwMatches;
};

BOOL
TextOrImageMatch(
    __in HWND hwnd,
    __in RegExp *pExpr
)
{
    BOOL fResult = FALSE;
    WCHAR  wszText[1024] = L"";

    if( GetWindowTextW( hwnd, wszText, ARRAYSIZE( wszText ) ) == 0 )
    {
        DebugOut( L"GetWindowTextW for %p failed with %d\n", hwnd, GetLastError() );
        goto Exit;
    }
    
    DebugOut( L"Text for %p is '%s'\n", hwnd, wszText );

    if( std::regex_search( wszText, *pExpr ) )
    {
        DebugOut( L"Matched text for %p\n", hwnd );
        fResult = TRUE;
        goto Exit;
    }

    if( !GetProcessImageName( hwnd, wszText, ARRAYSIZE( wszText ) ) )
    {
        DebugOut( L"GetProcessImageName for %p failed\n", hwnd );
        goto Exit;
    }

    DebugOut( L"Image for %p is '%s'\n", hwnd, wszText );

    if( std::regex_search( wszText, *pExpr ) )
    {
        DebugOut( L"Matched image for %p\n", hwnd );
        fResult = TRUE;
        goto Exit;
    }

Exit:

    return fResult;
}

BOOL CALLBACK
FindMatchesEnumCallback(
    __in HWND hwnd,
    __in LPARAM lParam
)
{
    FindMatchesEnumContext *pContext = NULL;
    WINDOWINFO info = {};

    pContext = (FindMatchesEnumContext *) lParam;

    DebugOut( L"Attempt match for hwnd %p\n", hwnd );

    if( pContext->dwMatches >= pContext->dwMaxMatches )
    {
        DebugOut( L"Too many matches, skipping\n" );
        goto Exit;
    }

    info.cbSize = sizeof info;
    if( !GetWindowInfo( hwnd, &info ) )
    {
        DebugOut( L"GetWindowInfo for %p failed with %d\n", hwnd, GetLastError() );
        goto Exit;
    }

    if( ( info.dwStyle & VALID_WINDOW_STYLE_MASK ) != VALID_WINDOW_STYLE_VALUE )
    {
        DebugOut( L"dwStyle for %p is 0x%08x, skipping\n", hwnd, info.dwStyle );
        goto Exit;
    }

    if( !TextOrImageMatch( hwnd, pContext->pExpr ) )
    {
        DebugOut( L"No match for %p\n", hwnd );
        goto Exit;
    }
    
    DebugOut( L"Match succeeded for %p\n", hwnd );

    pContext->phMatches[pContext->dwMatches] = hwnd;
    pContext->dwMatches++;

Exit:

    return TRUE;
}
    
int __cdecl
MatchesSortCallback(
    const void *e1,
    const void *e2
)
{
    const HWND *h1 = (const HWND *) e1;
    const HWND *h2 = (const HWND *) e2;

    if( *h1 == *h2 )
    {
        return 0;
    }
    if( *h1 > *h2 )
    {
        return 1;
    }
    return -1;
}

BOOL
CHotKey::FindMatches(
    __out_ecount(dwMatches) HWND *phMatches,
    __in DWORD dwMatches,
    __out DWORD *pdwActual
)
{
    BOOL fResult = FALSE;
    FindMatchesEnumContext   context = {};

    *pdwActual = 0;

    context.pExpr = m_pExpr;
    context.phMatches = phMatches;
    context.dwMaxMatches = dwMatches;
    context.dwMatches = 0;

    if( !EnumWindows( FindMatchesEnumCallback, (LPARAM) &context ) )
    {
        goto Exit;
    }

    qsort( phMatches, context.dwMatches, sizeof *phMatches, MatchesSortCallback );

    *pdwActual = context.dwMatches;
    fResult = TRUE;

Exit:

    return fResult;
}

CSwitcher::CSwitcher() :
    m_dwKeys( 0 ),
    m_dwLastKey( 0 )
{
    memset( m_rgpKeys, 0, sizeof m_rgpKeys );
}

CSwitcher::~CSwitcher()
{
    FreeKeys();
}

VOID
CSwitcher::FreeKeys()
{
    DWORD i = 0;
    
    for( i = 0; i < m_dwKeys; ++i )
    {
        UnregisterHotKey( m_hWnd, i + 1 );
        delete m_rgpKeys[i];
        m_rgpKeys[i] = NULL;
    }

    m_dwKeys = 0;
    m_dwLastKey = 0;
}

BOOL
CSwitcher::LoadFromRegistry()
{
    BOOL fResult = FALSE;
    HKEY hKeys = NULL;
    WCHAR pwszKey[256] = L"";
    WCHAR pwszExpr[2048] = L"";
    DWORD dwIndex = 0;
    DWORD dwType = 0;
    CHotKey *pKey = NULL;
    DWORD dwError = ERROR_SUCCESS;
    DWORD cchKey = 0;
    DWORD cbExpr = 0;
    DWORD cchExpr = 0;

    FreeKeys();

    DebugOut( L"Loading configuration from registry\n" );

    dwError = (DWORD) RegOpenKeyExW( HKEY_CURRENT_USER, L"Software\\Trunley\\HotSwitcher\\Keys", 0, KEY_READ, &hKeys );
    if( dwError != ERROR_SUCCESS )
    {
        DebugOut( L"RegOpenKeyExW(HKCU\\Software\\Trunley\\HotSwitcher\\Keys) failed with %d\n", dwError );
        hKeys = NULL;
        goto Exit;
    }

    for( dwIndex = 0; m_dwKeys < ARRAYSIZE( m_rgpKeys ); dwIndex++ )
    {
        cchKey = ARRAYSIZE( pwszKey );
        cbExpr = sizeof pwszExpr;
        dwError = (DWORD) RegEnumValueW( hKeys, dwIndex, pwszKey, &cchKey, NULL, &dwType, (BYTE *) pwszExpr, &cbExpr );
        if( dwError == ERROR_NO_MORE_ITEMS )
        {
            break;
        }

        if( dwError != ERROR_SUCCESS )
        {
            DebugOut( L"RegEnumValueW failed with %d\n", dwError );
            goto Exit;
        }

        if( dwType != REG_SZ )
        {
            DebugOut( L"Value %s is not REG_SZ, skipping\n", pwszKey );
            continue;
        }

        if( ( cbExpr % sizeof *pwszExpr ) != 0 )
        {
            DebugOut( L"Value %s has non-integral length, skipping\n", pwszKey );
            continue;
        }

        cchExpr = cbExpr / sizeof *pwszExpr;
        if( cchExpr >= ARRAYSIZE( pwszExpr ) )
        {
            DebugOut( L"Value %s has excessive length, skipping\n", pwszKey );
            continue;
        }
        pwszExpr[cchExpr] = 0;

        if( wcslen( pwszExpr ) == 0 )
        {
            DebugOut( L"Value %s has zero length, skipping\n", pwszKey );
            continue;
        }

        pKey = new CHotKey();
        if( pKey == NULL )
        {
            fwprintf( stderr, L"Failed to allocate new CHotKey\n" );
            goto Exit;
        }

        if( !pKey->Initialize( pwszKey, pwszExpr ) )
        {
            fwprintf( stderr, L"Failed to parse key '%s' (match '%s'), skipping\n", pwszKey, pwszExpr );
            delete pKey;
            pKey = NULL;
            continue;
        }

        DebugOut( L"Adding key '%s' match '%s'\n", pwszKey, pwszExpr );

        m_rgpKeys[m_dwKeys++] = pKey;
        pKey = NULL;
    }

    fResult = TRUE;

Exit:

    if( hKeys != NULL )
    {
        RegCloseKey( hKeys );
        hKeys = NULL;
    }

    delete pKey;
    pKey = NULL;

    return fResult;
}

BOOL
CSwitcher::RegisterKeys(
    __in HWND hWnd
)
{
    BOOL fResult = FALSE;
    DWORD i = 0;

    for( i = 0; i < m_dwKeys; ++i )
    {
        if( !RegisterHotKey( hWnd, i+1, m_rgpKeys[i]->m_dwMod, m_rgpKeys[i]->m_dwKey ) )
        {
            fwprintf( stderr, L"RegisterHotKey(%p, %d, 0x%02x, 0x%02x) failed with %d\n", hWnd, i+1, m_rgpKeys[i]->m_dwMod, m_rgpKeys[i]->m_dwKey, GetLastError() );
            goto Exit;
        }
    }

    DebugOut( L"Registered %d keys\n", m_dwKeys );
    
    fResult = TRUE;
    
Exit:

    return fResult;
}

BOOL
CSwitcher::ProcessHotKeyMessage(
    __in MSG  *pMsg
)
{
    BOOL fResult = FALSE;
    DWORD dwIndex = 0;
    DWORD dwLastKey = 0;
    CHotKey *pKey = NULL;

    dwIndex = ((DWORD) pMsg->wParam) - 1;
    if( dwIndex >= m_dwKeys )
    {
        goto Exit;
    }

    pKey = m_rgpKeys[dwIndex];

    dwLastKey = m_dwLastKey;
    m_dwLastKey = dwIndex;

    if( !pKey->Activate( dwLastKey == dwIndex ) )
    {
        goto Exit;
    }

    fResult = TRUE;

Exit:
    
    return fResult;
}
    

VOID ShowKeys(
)
{
    DWORD i = 0;
    WCHAR wszName[100] = L"";

    fwprintf( stderr, L"Examples:\n" );
    fwprintf( stderr, L"    CTRL+ALT+A        - Control, Alt and 'A'\n" );
    fwprintf( stderr, L"    ALT+F1            - Alt and F1\n" );
    fwprintf( stderr, L"    VOLUME_MUTE       - Mute\n\n");
    fwprintf( stderr, L"Modifier keys:" );
    for( i = 0; i < ARRAYSIZE( ModKeys ); ++i )
    {
        if( ( i % 4 ) == 0 )
        {
            fwprintf( stderr, L"\n    " );
        }

        StringCchCopyW( wszName, ARRAYSIZE( wszName ), ModKeys[i].pwszName );
        _wcsupr_s( wszName, ARRAYSIZE( wszName) );
        fwprintf( stderr, L"%-20s", wszName );
    }
    fwprintf( stderr, L"\n\nKeys:" );
    for( i = 0; i < ARRAYSIZE( Keys ); ++i )
    {
        if( ( i % 4 ) == 0 )
        {
            fwprintf( stderr, L"\n    " );
        }

        StringCchCopyW( wszName, ARRAYSIZE( wszName ), Keys[i].pwszName );
        _wcsupr_s( wszName, ARRAYSIZE( wszName) );
        fwprintf( stderr, L"%-20s", wszName );
    }
    fwprintf( stderr, L"\n" );
}


        
    
