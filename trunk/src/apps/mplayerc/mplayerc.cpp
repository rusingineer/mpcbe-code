/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2016 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include <thread>
#include <regex>
#include "AboutDlg.h"
#include <Tlhelp32.h>
#include "MainFrm.h"
#include <winternl.h>
#include <psapi.h>
#include "Ifo.h"
#include "MultiMonitor.h"
#include "../../DSUtil/WinAPIUtils.h"
#include "../../DSUtil/SysVersion.h"
#include "../../DSUtil/FileHandle.h"
#include "../../DSUtil/FileVersionInfo.h"
#include "../../DSUtil/HTTPAsync.h"
#include "PlayerYouTube.h"
#include <moreuuids.h>
#include <winddk/ntddcdvd.h>
#include <detours/detours.h>
#include <afxsock.h>
#include <atlsync.h>
#include <atlutil.h>
#include "UpdateChecker.h"

const LanguageResource CMPlayerCApp::languageResources[] = {
	{ID_LANGUAGE_ARMENIAN,				1067,	_T("Armenian"),					_T("hy")},
	{ID_LANGUAGE_BASQUE,				1069,	_T("Basque"),					_T("eu")},
	{ID_LANGUAGE_BELARUSIAN,			1059,	_T("Belarusian"),				_T("by")},
	{ID_LANGUAGE_CATALAN,				1027,	_T("Catalan"),					_T("ca")},
	{ID_LANGUAGE_CHINESE_SIMPLIFIED,	2052,	_T("Chinese (Simplified)"),		_T("sc")},
	{ID_LANGUAGE_CHINESE_TRADITIONAL,	3076,	_T("Chinese (Traditional)"),	_T("tc")},
	{ID_LANGUAGE_CZECH,					1029,	_T("Czech"),					_T("cz")},
	{ID_LANGUAGE_DUTCH,					1043,	_T("Dutch"),					_T("nl")},
	{ID_LANGUAGE_ENGLISH,				0,		_T("English"),					_T("en")},
	{ID_LANGUAGE_FRENCH,				1036,	_T("French"),					_T("fr")},
	{ID_LANGUAGE_GERMAN,				1031,	_T("German"),					_T("de")},
	{ID_LANGUAGE_GREEK,					1032,	_T("Greek"),					_T("el")},
	{ID_LANGUAGE_HEBREW,				1037,	_T("Hebrew"),					_T("he")},
	{ID_LANGUAGE_HUNGARIAN,				1038,	_T("Hungarian"),				_T("hu")},
	{ID_LANGUAGE_ITALIAN,				1040,	_T("Italian"),					_T("it")},
	{ID_LANGUAGE_JAPANESE,				1041,	_T("Japanese"),					_T("ja")},
	{ID_LANGUAGE_KOREAN,				1042,	_T("Korean"),					_T("kr")},
	{ID_LANGUAGE_POLISH,				1045,	_T("Polish"),					_T("pl")},
	{ID_LANGUAGE_PORTUGUESE_BR,			1046,	_T("Portuguese (Brazil)"),		_T("br")},
	{ID_LANGUAGE_ROMANIAN,				1048,	_T("Romanian"),					_T("ro")},
	{ID_LANGUAGE_RUSSIAN,				1049,	_T("Russian"),					_T("ru")},
	{ID_LANGUAGE_SLOVAK,				1053,	_T("Slovak"),					_T("sk")},
	{ID_LANGUAGE_SWEDISH,				1051,	_T("Swedish"),					_T("sv")},
	{ID_LANGUAGE_SPANISH,				1034,	_T("Spanish"),					_T("es")},
	{ID_LANGUAGE_TURKISH,				1055,	_T("Turkish"),					_T("tr")},
	{ID_LANGUAGE_UKRAINIAN,				1058,	_T("Ukrainian"),				_T("ua")},
};

const size_t CMPlayerCApp::languageResourcesCount = _countof(CMPlayerCApp::languageResources);

extern "C" {
	int mingw_app_type = 1;
}

typedef struct _PEB_FREE_BLOCK // Size = 8
{
	struct _PEB_FREE_BLOCK *Next;
	ULONG Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

typedef void (*PPEBLOCKROUTINE)(PVOID);

// PEB (Process Environment Block) data structure (FS:[0x30])
// Located at addr. 0x7FFDF000

typedef struct _PEB_NT // Size = 0x1E8
{
	BOOLEAN				InheritedAddressSpace;			//000
	BOOLEAN				ReadImageFileExecOptions;		//001
	BOOLEAN				BeingDebugged;				//002
	BOOLEAN				SpareBool;					//003 Allocation size
	HANDLE				Mutant;					//004
	HINSTANCE				ImageBaseAddress;				//008 Instance
	PPEB_LDR_DATA			LdrData;					//00C
	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters;			//010
	ULONG					SubSystemData;				//014
	HANDLE				ProcessHeap;				//018
	KSPIN_LOCK				FastPebLock;				//01C
	PPEBLOCKROUTINE			FastPebLockRoutine;			//020
	PPEBLOCKROUTINE			FastPebUnlockRoutine;			//024
	ULONG					EnvironmentUpdateCount;			//028
	PVOID *				KernelCallbackTable;			//02C
	PVOID					EventLogSection;				//030
	PVOID					EventLog;					//034
	PPEB_FREE_BLOCK			FreeList;					//038
	ULONG					TlsExpansionCounter;			//03C
	ULONG					TlsBitmap;					//040
	LARGE_INTEGER			TlsBitmapBits;				//044
	PVOID					ReadOnlySharedMemoryBase;		//04C
	PVOID					ReadOnlySharedMemoryHeap;		//050
	PVOID *				ReadOnlyStaticServerData;		//054
	PVOID					AnsiCodePageData;				//058
	PVOID					OemCodePageData;				//05C
	PVOID					UnicodeCaseTableData;			//060
	ULONG					NumberOfProcessors;			//064
	LARGE_INTEGER			NtGlobalFlag;				//068 Address of a local copy
	LARGE_INTEGER			CriticalSectionTimeout;			//070
	ULONG					HeapSegmentReserve;			//078
	ULONG					HeapSegmentCommit;			//07C
	ULONG					HeapDeCommitTotalFreeThreshold;	//080
	ULONG					HeapDeCommitFreeBlockThreshold;	//084
	ULONG					NumberOfHeaps;				//088
	ULONG					MaximumNumberOfHeaps;			//08C
	PVOID **				ProcessHeaps;				//090
	PVOID					GdiSharedHandleTable;			//094
	PVOID					ProcessStarterHelper;			//098
	PVOID					GdiDCAttributeList;			//09C
	KSPIN_LOCK				LoaderLock;					//0A0
	ULONG					OSMajorVersion;				//0A4
	ULONG					OSMinorVersion;				//0A8
	USHORT				OSBuildNumber;				//0AC
	USHORT				OSCSDVersion;				//0AE
	ULONG					OSPlatformId;				//0B0
	ULONG					ImageSubsystem;				//0B4
	ULONG					ImageSubsystemMajorVersion;		//0B8
	ULONG					ImageSubsystemMinorVersion;		//0BC
	ULONG					ImageProcessAffinityMask;		//0C0
	ULONG					GdiHandleBuffer[0x22];			//0C4
	ULONG					PostProcessInitRoutine;			//14C
	ULONG					TlsExpansionBitmap;			//150
	UCHAR					TlsExpansionBitmapBits[0x80];		//154
	ULONG					SessionId;					//1D4
	void *				AppCompatInfo;				//1D8
	UNICODE_STRING			CSDVersion;					//1DC
} PEB_NT, *PPEB_NT;

HICON LoadIcon(CString fn, bool fSmall)
{
	if (fn.IsEmpty()) {
		return NULL;
	}

	CString ext = fn.Left(fn.Find(_T("://"))+1).TrimRight(':');
	if (ext.IsEmpty() || !ext.CompareNoCase(_T("file"))) {
		ext = _T(".") + fn.Mid(fn.ReverseFind('.')+1);
	}

	CSize size(fSmall?16:32, fSmall?16:32);

	if (!ext.CompareNoCase(_T(".ifo"))) {
		if (HICON hIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_DVD), IMAGE_ICON, size.cx, size.cy, 0)) {
			return hIcon;
		}
	}

	if (!ext.CompareNoCase(_T(".cda"))) {
		if (HICON hIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_AUDIOCD), IMAGE_ICON, size.cx, size.cy, 0)) {
			return hIcon;
		}
	}

	if ((CString(fn).MakeLower().Find(_T("://"))) >= 0) {
		if (HICON hIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, size.cx, size.cy, 0)) {
			return hIcon;
		}

		return NULL;
	}

	TCHAR buff[MAX_PATH];
	lstrcpy(buff, fn.GetBuffer());

	SHFILEINFO sfi;
	ZeroMemory(&sfi, sizeof(sfi));

	if (SUCCEEDED(SHGetFileInfo(buff, 0, &sfi, sizeof(sfi), (fSmall ? SHGFI_SMALLICON : SHGFI_LARGEICON) |SHGFI_ICON)) && sfi.hIcon) {
		return sfi.hIcon;
	}

	ULONG len;
	HICON hIcon = NULL;

	do {
		CRegKey key;

		CString RegPathAssociated;
		RegPathAssociated.Format(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%ws\\UserChoice"), ext);

		if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, RegPathAssociated, KEY_READ)) {
			len = _countof(buff);
			memset(buff, 0, sizeof(buff));

			CString ext_Associated = ext;
			if (ERROR_SUCCESS == key.QueryStringValue(_T("Progid"), buff, &len) && !(ext_Associated = buff).Trim().IsEmpty()) {
				ext = ext_Associated;
			}
		}

		if (ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, ext + _T("\\DefaultIcon"), KEY_READ)) {
			if (ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, ext, KEY_READ)) {
				break;
			}

			len = _countof(buff);
			memset(buff, 0, sizeof(buff));
			if (ERROR_SUCCESS != key.QueryStringValue(NULL, buff, &len) || (ext = buff).Trim().IsEmpty()) {
				break;
			}

			if (ERROR_SUCCESS != key.Open(HKEY_CLASSES_ROOT, ext + _T("\\DefaultIcon"), KEY_READ)) {
				break;
			}
		}

		CString icon;

		len = _countof(buff);
		memset(buff, 0, sizeof(buff));
		if (ERROR_SUCCESS != key.QueryStringValue(NULL, buff, &len) || (icon = buff).Trim().IsEmpty()) {
			break;
		}

		int i = icon.ReverseFind(',');
		if (i < 0) {
			break;
		}

		int id = 0;
		if (_stscanf_s(icon.Mid(i+1), _T("%d"), &id) != 1) {
			break;
		}

		icon = icon.Left(i);
		icon.Replace(_T("\""), _T(""));

		hIcon = NULL;
		UINT cnt = fSmall
				   ? ExtractIconEx(icon, id, NULL, &hIcon, 1)
				   : ExtractIconEx(icon, id, &hIcon, NULL, 1);
		UNREFERENCED_PARAMETER(cnt);
		if (hIcon) {
			return hIcon;
		}
	} while (0);

	return (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_UNKNOWN), IMAGE_ICON, size.cx, size.cy, 0);
}

bool LoadType(CString fn, CString& type)
{
	bool found = false;

	if (!fn.IsEmpty()) {
		CString ext = fn.Left(fn.Find(_T("://"))+1).TrimRight(':');
		if (ext.IsEmpty() || !ext.CompareNoCase(_T("file"))) {
			ext = _T(".") + fn.Mid(fn.ReverseFind('.')+1);
		}

		// Try MPC-BE's internal formats list
		CMediaFormatCategory* mfc = AfxGetAppSettings().m_Formats.FindMediaByExt(ext);

		if (mfc != NULL) {
			found = true;
			type = mfc->GetDescription();
		} else { // Fallback to registry
			CRegKey key;

			CString tmp;
			CString mplayerc_ext = _T("mplayerc") + ext;

			if (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, mplayerc_ext)) {
				tmp = mplayerc_ext;
			}

			if (!tmp.IsEmpty() || ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, ext)) {
				found = true;

				if (tmp.IsEmpty()) {
					tmp = ext;
				}

				TCHAR buff[256] = { 0 };
				ULONG len;

				while (ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, tmp)) {
					len = _countof(buff);
					memset(buff, 0, sizeof(buff));

					if (ERROR_SUCCESS != key.QueryStringValue(NULL, buff, &len)) {
						break;
					}

					CString str(buff);
					str.Trim();

					if (str.IsEmpty() || str == tmp) {
						break;
					}

					tmp = str;
				}

				type = tmp;
			}
		}
	}

	return found;
}

bool LoadResource(UINT resid, CStringA& str, LPCTSTR restype)
{
	str.Empty();
	HRSRC hrsrc = FindResource(AfxGetApp()->m_hInstance, MAKEINTRESOURCE(resid), restype);
	if (!hrsrc) {
		return false;
	}
	HGLOBAL hGlobal = LoadResource(AfxGetApp()->m_hInstance, hrsrc);
	if (!hGlobal) {
		return false;
	}
	DWORD size = SizeofResource(AfxGetApp()->m_hInstance, hrsrc);
	if (!size) {
		return false;
	}
	memcpy(str.GetBufferSetLength(size), LockResource(hGlobal), size);
	return true;
}

WORD AssignedToCmd(UINT keyOrMouseValue, bool bIsFullScreen, bool bCheckMouse)
{
	WORD assignTo = 0;
	CAppSettings& s = AfxGetAppSettings();

	POSITION pos = s.wmcmds.GetHeadPosition();
	while (pos && !assignTo) {
		wmcmd& wc = s.wmcmds.GetNext(pos);

		if (bCheckMouse) {
			if (bIsFullScreen) {
				if (wc.mouseFS == keyOrMouseValue) {
					assignTo = wc.cmd;
				}
			} else if (wc.mouse == keyOrMouseValue) {
				assignTo = wc.cmd;
			}
		} else if (wc.key == keyOrMouseValue) {
			assignTo = wc.cmd;
		}
	}

	return assignTo;
}

/////////////////////////////////////////////////////////////////////////////
// CMPlayerCApp

BEGIN_MESSAGE_MAP(CMPlayerCApp, CWinApp)
	//{{AFX_MSG_MAP(CMPlayerCApp)
	ON_COMMAND(ID_HELP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_EXIT, OnFileExit)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP_SHOWCOMMANDLINESWITCHES, OnHelpShowcommandlineswitches)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMPlayerCApp construction

CMPlayerCApp::CMPlayerCApp()
	: m_bProfileInitialized(false)
	, m_bQueuedProfileFlush(false)
	, m_dwProfileLastAccessTick(0)
	, GetRemoteControlCode(GetRemoteControlCodeMicrosoft)
{
}

CMPlayerCApp::~CMPlayerCApp()
{
	// Wait for any pending I/O operations to be canceled
	while (WAIT_IO_COMPLETION == SleepEx(0, TRUE));
}

BOOL CMPlayerCApp::OnIdle(LONG lCount)
{
	BOOL ret = __super::OnIdle(lCount);
	if (!ret) {
		FlushProfile(false);
	}

	return ret;
}

void CMPlayerCApp::InitProfile()
{
	std::lock_guard<std::recursive_mutex> lock(m_profileMutex);

	if (!m_pszRegistryKey) {
		// Don't reread mpc-be.ini if the cache needs to be flushed or it was accessed recently
		if (m_bProfileInitialized && (m_bQueuedProfileFlush || GetTickCount() - m_dwProfileLastAccessTick < 100)) {
			m_dwProfileLastAccessTick = GetTickCount();
			return;
		}

		m_bProfileInitialized = true;
		m_dwProfileLastAccessTick = GetTickCount();

		ASSERT(m_pszProfileName);
		if (!::PathFileExists(m_pszProfileName)) {
			return;
		}

		FILE* fp;
		int fpStatus;
		do { // Open mpc-be.ini in UNICODE mode, retry if it is already being used by another process
			fp = _tfsopen(m_pszProfileName, _T("r, ccs=UNICODE"), _SH_SECURE);
			if (fp || (GetLastError() != ERROR_SHARING_VIOLATION)) {
				break;
			}
			Sleep(100);
		} while (true);
		if (!fp) {
			ASSERT(FALSE);
			return;
		}
		if (_ftell_nolock(fp) == 0L) {
			// No BOM was consumed, assume mpc-be.ini is ANSI encoded
			fpStatus = fclose(fp);
			ASSERT(fpStatus == 0);
			do { // Reopen mpc-be.ini in ANSI mode, retry if it is already being used by another process
				fp = _tfsopen(m_pszProfileName, _T("r"), _SH_SECURE);
				if (fp || (GetLastError() != ERROR_SHARING_VIOLATION)) {
					break;
				}
				Sleep(100);
			} while (true);
			if (!fp) {
				ASSERT(FALSE);
				return;
			}
		}

		CStdioFile file(fp);

		ASSERT(!m_bQueuedProfileFlush);
		m_ProfileMap.clear();

		CString line, section, var, val;
		while (file.ReadString(line)) {
			// Parse mpc-be.ini file, this parser:
			//  - doesn't trim whitespaces
			//  - doesn't remove quotation marks
			//  - omits keys with empty names
			//  - omits unnamed sections
			int pos = 0;
			if (line[0] == _T('[')) {
				pos = line.Find(_T(']'));
				if (pos == -1) {
					continue;
				}
				section = line.Mid(1, pos - 1);
			} else if (line[0] != _T(';')) {
				pos = line.Find(_T('='));
				if (pos == -1) {
					continue;
				}
				var = line.Mid(0, pos);
				val = line.Mid(pos + 1);
				if (!section.IsEmpty() && !var.IsEmpty()) {
					m_ProfileMap[section][var] = val;
				}
			}
		}
		fpStatus = fclose(fp);
		ASSERT(fpStatus == 0);

		m_dwProfileLastAccessTick = GetTickCount();
	}
}

void CMPlayerCApp::FlushProfile(bool bForce/* = true*/)
{
    std::lock_guard<std::recursive_mutex> lock(m_profileMutex);

    if (!m_pszRegistryKey) {
        if (!bForce && !m_bQueuedProfileFlush) {
            return;
        }

        m_bQueuedProfileFlush = false;

        ASSERT(m_bProfileInitialized);
        ASSERT(m_pszProfileName);

        FILE* fp;
        int fpStatus;
        do { // Open mpc-be.ini, retry if it is already being used by another process
            fp = _tfsopen(m_pszProfileName, _T("w, ccs=UTF-8"), _SH_SECURE);
            if (fp || (GetLastError() != ERROR_SHARING_VIOLATION)) {
                break;
            }
            Sleep(100);
        } while (true);
        if (!fp) {
            ASSERT(FALSE);
            return;
        }
        CStdioFile file(fp);
        CString line;
        try {
            file.WriteString(_T("; MPC-BE\n"));
            for (auto it1 = m_ProfileMap.begin(); it1 != m_ProfileMap.end(); ++it1) {
                line.Format(_T("[%s]\n"), it1->first);
                file.WriteString(line);
                for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2) {
                    line.Format(_T("%s=%s\n"), it2->first, it2->second);
                    file.WriteString(line);
                }
            }
        } catch (CFileException& e) {
            // Fail silently if disk is full
            UNREFERENCED_PARAMETER(e);
            ASSERT(FALSE);
        }
        fpStatus = fclose(fp);
        ASSERT(fpStatus == 0);
    }
}

BOOL CMPlayerCApp::GetProfileBinary(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPBYTE* ppData, UINT* pBytes)
{
    std::lock_guard<std::recursive_mutex> lock(m_profileMutex);

    if (m_pszRegistryKey) {
        return CWinApp::GetProfileBinary(lpszSection, lpszEntry, ppData, pBytes);
    } else {
        if (!lpszSection || !lpszEntry || !ppData || !pBytes) {
            ASSERT(FALSE);
            return FALSE;
        }
        CString sectionStr(lpszSection);
        CString keyStr(lpszEntry);
        if (sectionStr.IsEmpty() || keyStr.IsEmpty()) {
            ASSERT(FALSE);
            return FALSE;
        }
        CString valueStr;

        InitProfile();
        auto it1 = m_ProfileMap.find(sectionStr);
        if (it1 != m_ProfileMap.end()) {
            auto it2 = it1->second.find(keyStr);
            if (it2 != it1->second.end()) {
                valueStr = it2->second;
            }
        }
        if (valueStr.IsEmpty()) {
            return FALSE;
        }
        int length = valueStr.GetLength();
        // Encoding: each 4-bit sequence is coded in one character, from 'A' for 0x0 to 'P' for 0xf
        if (length % 2) {
            ASSERT(FALSE);
            return FALSE;
        }
        for (int i = 0; i < length; i++) {
            if (valueStr[i] < 'A' || valueStr[i] > 'P') {
                ASSERT(FALSE);
                return FALSE;
            }
        }
        *pBytes = length / 2;
        *ppData = new(std::nothrow) BYTE[*pBytes];
        if (!(*ppData)) {
            ASSERT(FALSE);
            return FALSE;
        }
        for (UINT i = 0; i < *pBytes; i++) {
            (*ppData)[i] = BYTE((valueStr[i * 2] - 'A') | ((valueStr[i * 2 + 1] - 'A') << 4));
        }
        return TRUE;
    }
}

UINT CMPlayerCApp::GetProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nDefault)
{
    std::lock_guard<std::recursive_mutex> lock(m_profileMutex);

    int res = nDefault;
    if (m_pszRegistryKey) {
        res = CWinApp::GetProfileInt(lpszSection, lpszEntry, nDefault);
    } else {
        if (!lpszSection || !lpszEntry) {
            ASSERT(FALSE);
            return res;
        }
        CString sectionStr(lpszSection);
        CString keyStr(lpszEntry);
        if (sectionStr.IsEmpty() || keyStr.IsEmpty()) {
            ASSERT(FALSE);
            return res;
        }

        InitProfile();
        auto it1 = m_ProfileMap.find(sectionStr);
        if (it1 != m_ProfileMap.end()) {
            auto it2 = it1->second.find(keyStr);
            if (it2 != it1->second.end()) {
                res = _ttoi(it2->second);
            }
        }
    }
    return res;
}

CString CMPlayerCApp::GetProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault/* = NULL*/)
{
    std::lock_guard<std::recursive_mutex> lock(m_profileMutex);

    CString res;
    if (m_pszRegistryKey) {
        res = CWinApp::GetProfileString(lpszSection, lpszEntry, lpszDefault);
    } else {
        if (!lpszSection || !lpszEntry) {
            ASSERT(FALSE);
            return res;
        }
        CString sectionStr(lpszSection);
        CString keyStr(lpszEntry);
        if (sectionStr.IsEmpty() || keyStr.IsEmpty()) {
            ASSERT(FALSE);
            return res;
        }
        if (lpszDefault) {
            res = lpszDefault;
        }

        InitProfile();
        auto it1 = m_ProfileMap.find(sectionStr);
        if (it1 != m_ProfileMap.end()) {
            auto it2 = it1->second.find(keyStr);
            if (it2 != it1->second.end()) {
                res = it2->second;
            }
        }
    }
    return res;
}

BOOL CMPlayerCApp::WriteProfileBinary(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPBYTE pData, UINT nBytes)
{
    std::lock_guard<std::recursive_mutex> lock(m_profileMutex);

    if (m_pszRegistryKey) {
        return CWinApp::WriteProfileBinary(lpszSection, lpszEntry, pData, nBytes);
    } else {
        if (!lpszSection || !lpszEntry || !pData || !nBytes) {
            ASSERT(FALSE);
            return FALSE;
        }
        CString sectionStr(lpszSection);
        CString keyStr(lpszEntry);
        if (sectionStr.IsEmpty() || keyStr.IsEmpty()) {
            ASSERT(FALSE);
            return FALSE;
        }
        CString valueStr;

        TCHAR* buffer = valueStr.GetBufferSetLength(nBytes * 2);
        // Encoding: each 4-bit sequence is coded in one character, from 'A' for 0x0 to 'P' for 0xf
        for (UINT i = 0; i < nBytes; i++) {
            buffer[i * 2] = 'A' + (pData[i] & 0xf);
            buffer[i * 2 + 1] = 'A' + (pData[i] >> 4 & 0xf);
        }
        valueStr.ReleaseBufferSetLength(nBytes * 2);

        InitProfile();
        CString& old = m_ProfileMap[sectionStr][keyStr];
        if (old != valueStr) {
            old = valueStr;
            m_bQueuedProfileFlush = true;
        }
        return TRUE;
    }
}

BOOL CMPlayerCApp::WriteProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nValue)
{
    std::lock_guard<std::recursive_mutex> lock(m_profileMutex);

    if (m_pszRegistryKey) {
        return CWinApp::WriteProfileInt(lpszSection, lpszEntry, nValue);
    } else {
        if (!lpszSection || !lpszEntry) {
            ASSERT(FALSE);
            return FALSE;
        }
        CString sectionStr(lpszSection);
        CString keyStr(lpszEntry);
        if (sectionStr.IsEmpty() || keyStr.IsEmpty()) {
            ASSERT(FALSE);
            return FALSE;
        }
        CString valueStr;
        valueStr.Format(_T("%d"), nValue);

        InitProfile();
        CString& old = m_ProfileMap[sectionStr][keyStr];
        if (old != valueStr) {
            old = valueStr;
            m_bQueuedProfileFlush = true;
        }
        return TRUE;
    }
}

BOOL CMPlayerCApp::WriteProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszValue)
{
    std::lock_guard<std::recursive_mutex> lock(m_profileMutex);

    if (m_pszRegistryKey) {
        return CWinApp::WriteProfileString(lpszSection, lpszEntry, lpszValue);
    } else {
        if (!lpszSection) {
            ASSERT(FALSE);
            return FALSE;
        }
        CString sectionStr(lpszSection);
        if (sectionStr.IsEmpty()) {
            ASSERT(FALSE);
            return FALSE;
        }
        CString keyStr(lpszEntry);
        if (lpszEntry && keyStr.IsEmpty()) {
            ASSERT(FALSE);
            return FALSE;
        }

        InitProfile();

        // Mimic CWinApp::WriteProfileString() behavior
        if (lpszEntry) {
            if (lpszValue) {
                CString& old = m_ProfileMap[sectionStr][keyStr];
                if (old != lpszValue) {
                    old = lpszValue;
                    m_bQueuedProfileFlush = true;
                }
            } else { // Delete key
                auto it = m_ProfileMap.find(sectionStr);
                if (it != m_ProfileMap.end()) {
                    if (it->second.erase(keyStr)) {
                        m_bQueuedProfileFlush = true;
                    }
                }
            }
        } else { // Delete section
            if (m_ProfileMap.erase(sectionStr)) {
                m_bQueuedProfileFlush = true;
            }
        }
        return TRUE;
    }
}

bool CMPlayerCApp::HasProfileEntry(LPCTSTR lpszSection, LPCTSTR lpszEntry)
{
    std::lock_guard<std::recursive_mutex> lock(m_profileMutex);

    bool ret = false;
    if (m_pszRegistryKey) {
        if (HKEY hAppKey = GetAppRegistryKey()) {
            HKEY hSectionKey;
            if (RegOpenKeyEx(hAppKey, lpszSection, 0, KEY_READ, &hSectionKey) == ERROR_SUCCESS) {
                LONG lResult = RegQueryValueEx(hSectionKey, lpszEntry, NULL, NULL, NULL, NULL);
                ret = (lResult == ERROR_SUCCESS);
                VERIFY(RegCloseKey(hSectionKey) == ERROR_SUCCESS);
            }
            VERIFY(RegCloseKey(hAppKey) == ERROR_SUCCESS);
        } else {
            ASSERT(FALSE);
        }
    } else {
        InitProfile();
        auto it1 = m_ProfileMap.find(lpszSection);
        if (it1 != m_ProfileMap.end()) {
            auto& sectionMap = it1->second;
            auto it2 = sectionMap.find(lpszEntry);
            ret = (it2 != sectionMap.end());
        }
    }
    return ret;
}

void CMPlayerCApp::ShowCmdlnSwitches() const
{
	CString s;

	if (m_s.nCLSwitches&CLSW_UNRECOGNIZEDSWITCH) {
		CAtlList<CString> sl;
		for (int i = 0; i < __argc; i++) {
			sl.AddTail(__targv[i]);
		}
		s += ResStr(IDS_UNKNOWN_SWITCH) + Implode(sl, ' ') + _T("\n\n");
	}

	s += ResStr(IDS_USAGE);

	AfxMessageBox(s, MB_ICONINFORMATION | MB_OK);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMPlayerCApp object

CMPlayerCApp theApp;

HWND g_hWnd = NULL;

bool CMPlayerCApp::StoreSettingsToIni()
{
	free((void*)m_pszRegistryKey);
	m_pszRegistryKey = NULL;
	free((void*)m_pszProfileName);
	m_pszProfileName = _tcsdup(GetIniPath());

	if (!::PathFileExists(m_pszProfileName)) {
		// Create an empty mpc-be.ini file to be sure that the function IsIniValid() works correctly
		FILE* fp;
		int fpStatus;
		do { // Open mpc-be.ini, retry if it is already being used by another process
			fp = _tfsopen(m_pszProfileName, _T("w, ccs=UTF-8"), _SH_SECURE);
			if (fp || (GetLastError() != ERROR_SHARING_VIOLATION)) {
				break;
			}
			Sleep(100);
		} while (true);
		if (!fp) {
			ASSERT(FALSE);
		} else {
			fpStatus = fclose(fp);
			ASSERT(fpStatus == 0);
		}
	}

	return true;
}

bool CMPlayerCApp::StoreSettingsToRegistry()
{
	free((void*)m_pszRegistryKey);
	m_pszRegistryKey = NULL;

	SetRegistryKey(_T(""));

	return true;
}

CString CMPlayerCApp::GetIniPath() const
{
	CString path = GetProgramPath();
	path = path.Left(path.ReverseFind('.') + 1) + _T("ini");
	return path;
}

bool CMPlayerCApp::IsIniValid() const
{
	return !!::PathFileExists(GetIniPath());
}

bool CMPlayerCApp::GetAppSavePath(CString& path)
{
	path.Empty();

	if (IsIniValid()) { // If settings ini file found, store stuff in the same folder as the exe file
		path = GetProgramDir();
	} else {
		HRESULT hr = SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path.GetBuffer(_MAX_PATH));
		path.ReleaseBuffer();
		if (FAILED(hr)) {
			return false;
		}
		CPath p;
		p.Combine(path, _T("MPC-BE"));
		path = AddSlash(p);
	}

	return true;
}

bool CMPlayerCApp::ChangeSettingsLocation(bool useIni)
{
	bool success;

	CString oldpath;
	AfxGetMyApp()->GetAppSavePath(oldpath);

	// Load favorites so that they can be correctly saved to the new location
	CAtlList<CString> filesFav, DVDsFav, devicesFav;
	AfxGetAppSettings().GetFav(FAV_FILE, filesFav);
	AfxGetAppSettings().GetFav(FAV_DVD, DVDsFav);
	AfxGetAppSettings().GetFav(FAV_DEVICE, devicesFav);

	if (useIni) {
		success = StoreSettingsToIni();
	} else {
		success = StoreSettingsToRegistry();
		_tremove(GetIniPath());
	}

	if (success && oldpath.GetLength() > 0) {
		DeleteFile(oldpath + _T("default.mpcpl"));

		WIN32_FIND_DATA wfd;
		HANDLE hFile = FindFirstFile(oldpath + _T("Shaders\\*.hlsl"), &wfd);
		if (hFile != INVALID_HANDLE_VALUE) {
			do {
				DeleteFile(oldpath + _T("Shaders\\") + wfd.cFileName);
			} while (FindNextFile(hFile, &wfd));
			FindClose(hFile);
		}
		::RemoveDirectory(oldpath + _T("Shaders"));
	}
	// Ensure the shaders are properly saved
	AfxGetAppSettings().fShadersNeedSave = true;

	// Save favorites to the new location
	AfxGetAppSettings().SetFav(FAV_FILE, filesFav);
	AfxGetAppSettings().SetFav(FAV_DVD, DVDsFav);
	AfxGetAppSettings().SetFav(FAV_DEVICE, devicesFav);

	// Save external filters to the new location
	m_s.SaveExternalFilters();

	// Write settings immediately
	m_s.SaveSettings();

	return success;
}

void CMPlayerCApp::ExportSettings()
{
	CString ext = IsIniValid() ? _T("ini") : _T("reg");
	CString ext_list;
	ext_list.Format(L"Export files (*.%s)|*.%s|", ext, ext);

	CFileDialog fileSaveDialog(FALSE, 0, L"mpc-be-settings." + ext,
							   OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
							   ext_list);

	if (fileSaveDialog.DoModal() == IDOK) {
		CString savePath = fileSaveDialog.GetPathName();
		if (ext.CompareNoCase(fileSaveDialog.GetFileExt()) != 0) {
			savePath.Append(_T(".") + ext);
		}

		bool success = false;
		AfxGetAppSettings().SaveSettings();

		if (IsIniValid()) {
			success = !!CopyFile(GetIniPath(), savePath, FALSE);
		} else {
			CString regKey;
			regKey.Format(_T("Software\\%s"), m_pszProfileName);

			FILE* fStream;
			errno_t error = _tfopen_s(&fStream, savePath, _T("wt,ccs=UNICODE"));
			CStdioFile file(fStream);
			file.WriteString(_T("Windows Registry Editor Version 5.00\n\n"));

			success = !error && ExportRegistryKey(file, HKEY_CURRENT_USER, regKey);

			file.Close();
		}

		if (success) {
			MessageBox(GetMainWnd()->m_hWnd, ResStr(IDS_EXPORT_SETTINGS_SUCCESS), ResStr(IDS_EXPORT_SETTINGS), MB_ICONINFORMATION | MB_OK);
		} else {
			MessageBox(GetMainWnd()->m_hWnd, ResStr(IDS_EXPORT_SETTINGS_FAILED), ResStr(IDS_EXPORT_SETTINGS), MB_ICONERROR | MB_OK);
		}
	}
}

void CMPlayerCApp::PreProcessCommandLine()
{
	m_cmdln.RemoveAll();

	for (int i = 1; i < __argc; i++) {
		m_cmdln.AddTail(CString(__targv[i]).Trim(_T(" \"")));
	}
}

BOOL CMPlayerCApp::SendCommandLine(HWND hWnd)
{
	if (m_cmdln.IsEmpty()) {
		return FALSE;
	}

	int bufflen = sizeof(DWORD);

	POSITION pos = m_cmdln.GetHeadPosition();
	while (pos) {
		bufflen += (m_cmdln.GetNext(pos).GetLength()+1)*sizeof(TCHAR);
	}

	CAutoVectorPtr<BYTE> buff;
	if (!buff.Allocate(bufflen)) {
		return FALSE;
	}

	BYTE* p = buff;

	*(DWORD*)p = m_cmdln.GetCount();
	p += sizeof(DWORD);

	pos = m_cmdln.GetHeadPosition();
	while (pos) {
		CString s = m_cmdln.GetNext(pos);
		int len = (s.GetLength()+1)*sizeof(TCHAR);
		memcpy(p, s, len);
		p += len;
	}

	COPYDATASTRUCT cds;
	cds.dwData = 0x6ABE51;
	cds.cbData = bufflen;
	cds.lpData = (void*)(BYTE*)buff;
	return SendMessage(hWnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
}

/////////////////////////////////////////////////////////////////////////////
// CMPlayerCApp initialization

BOOL (__stdcall * Real_IsDebuggerPresent)(void)
	= IsDebuggerPresent;

LONG (__stdcall * Real_ChangeDisplaySettingsExA)(LPCSTR a0,
		LPDEVMODEA a1,
		HWND a2,
		DWORD a3,
		LPVOID a4)
	= ChangeDisplaySettingsExA;

LONG (__stdcall * Real_ChangeDisplaySettingsExW)(LPCWSTR a0,
		LPDEVMODEW a1,
		HWND a2,
		DWORD a3,
		LPVOID a4)
	= ChangeDisplaySettingsExW;

HANDLE (__stdcall * Real_CreateFileA)(LPCSTR a0,
									  DWORD a1,
									  DWORD a2,
									  LPSECURITY_ATTRIBUTES a3,
									  DWORD a4,
									  DWORD a5,
									  HANDLE a6)
	= CreateFileA;

HANDLE (__stdcall * Real_CreateFileW)(LPCWSTR a0,
									  DWORD a1,
									  DWORD a2,
									  LPSECURITY_ATTRIBUTES a3,
									  DWORD a4,
									  DWORD a5,
									  HANDLE a6)
	= CreateFileW;

BOOL (__stdcall * Real_DeviceIoControl)(HANDLE a0,
										DWORD a1,
										LPVOID a2,
										DWORD a3,
										LPVOID a4,
										DWORD a5,
										LPDWORD a6,
										LPOVERLAPPED a7)
	= DeviceIoControl;

MMRESULT  (__stdcall * Real_mixerSetControlDetails)( HMIXEROBJ hmxobj,
		LPMIXERCONTROLDETAILS pmxcd,
		DWORD fdwDetails)
	= mixerSetControlDetails;

typedef NTSTATUS (WINAPI *FUNC_NTQUERYINFORMATIONPROCESS)(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
static FUNC_NTQUERYINFORMATIONPROCESS		Real_NtQueryInformationProcess = NULL;

/*
NTSTATUS (* Real_NtQueryInformationProcess) (HANDLE				ProcessHandle,
											 PROCESSINFOCLASS	ProcessInformationClass,
											 PVOID				ProcessInformation,
											 ULONG				ProcessInformationLength,
											 PULONG				ReturnLength)
	= NULL;
*/

BOOL WINAPI Mine_IsDebuggerPresent()
{
	DbgLog((LOG_TRACE, 3, L"Oops, somebody was trying to be naughty! (called IsDebuggerPresent)"));
	return FALSE;
}

NTSTATUS WINAPI Mine_NtQueryInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength)
{
	NTSTATUS nRet;

	nRet = Real_NtQueryInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);

	if (ProcessInformationClass == ProcessBasicInformation) {
		PROCESS_BASIC_INFORMATION*		pbi = (PROCESS_BASIC_INFORMATION*)ProcessInformation;
		PEB_NT*							pPEB;
		PEB_NT							PEB;

		pPEB = (PEB_NT*)pbi->PebBaseAddress;
		ReadProcessMemory(ProcessHandle, pPEB, &PEB, sizeof(PEB), NULL);
		PEB.BeingDebugged = 0;
		WriteProcessMemory(ProcessHandle, pPEB, &PEB, sizeof(PEB), NULL);
	} else if (ProcessInformationClass == 7) { // ProcessDebugPort
		BOOL*		pDebugPort = (BOOL*)ProcessInformation;
		*pDebugPort = FALSE;
	}

	return nRet;
}

LONG WINAPI Mine_ChangeDisplaySettingsEx(LONG ret, DWORD dwFlags, LPVOID lParam)
{
	if (dwFlags&CDS_VIDEOPARAMETERS) {
		VIDEOPARAMETERS* vp = (VIDEOPARAMETERS*)lParam;

		if (vp->Guid == GUIDFromCString(_T("{02C62061-1097-11d1-920F-00A024DF156E}"))
				&& (vp->dwFlags&VP_FLAGS_COPYPROTECT)) {
			if (vp->dwCommand == VP_COMMAND_GET) {
				if ((vp->dwTVStandard&VP_TV_STANDARD_WIN_VGA)
						&& vp->dwTVStandard != VP_TV_STANDARD_WIN_VGA) {
					DbgLog((LOG_TRACE, 3, L"Ooops, tv-out enabled? macrovision checks suck..."));
					vp->dwTVStandard = VP_TV_STANDARD_WIN_VGA;
				}
			} else if (vp->dwCommand == VP_COMMAND_SET) {
				DbgLog((LOG_TRACE, 3, L"Ooops, as I already told ya, no need for any macrovision bs here"));
				return 0;
			}
		}
	}

	return ret;
}

LONG WINAPI Mine_ChangeDisplaySettingsExA(LPCSTR lpszDeviceName, LPDEVMODEA lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam)
{
	return Mine_ChangeDisplaySettingsEx(
			   Real_ChangeDisplaySettingsExA(lpszDeviceName, lpDevMode, hwnd, dwFlags, lParam),
			   dwFlags,
			   lParam);
}

LONG WINAPI Mine_ChangeDisplaySettingsExW(LPCWSTR lpszDeviceName, LPDEVMODEW lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam)
{
	return Mine_ChangeDisplaySettingsEx(
			   Real_ChangeDisplaySettingsExW(lpszDeviceName, lpDevMode, hwnd, dwFlags, lParam),
			   dwFlags,
			   lParam);
}

HANDLE WINAPI Mine_CreateFileA(LPCSTR p1, DWORD p2, DWORD p3, LPSECURITY_ATTRIBUTES p4, DWORD p5, DWORD p6, HANDLE p7)
{
	//CStringA fn(p1);
	//fn.MakeLower();
	//int i = fn.Find(".part");
	//if (i > 0 && i == fn.GetLength() - 5)
	p3 |= FILE_SHARE_WRITE;

	return Real_CreateFileA(p1, p2, p3, p4, p5, p6, p7);
}

BOOL CreateFakeVideoTS(LPCWSTR strIFOPath, LPWSTR strFakeFile, size_t nFakeFileSize)
{
	BOOL		bRet = FALSE;
	WCHAR		szTempPath[_MAX_PATH];
	WCHAR		strFileName[_MAX_PATH];
	WCHAR		strExt[_MAX_EXT];
	CIfo		Ifo;

	if (!GetTempPathW(_MAX_PATH, szTempPath)) {
		return FALSE;
	}

	_wsplitpath_s (strIFOPath, NULL, 0, NULL, 0, strFileName, _countof(strFileName), strExt, _countof(strExt));
	_snwprintf_s  (strFakeFile, nFakeFileSize, _TRUNCATE, L"%sMPC%s%s", szTempPath, strFileName, strExt);

	if (Ifo.OpenFile (strIFOPath) &&
			Ifo.RemoveUOPs()  &&
			Ifo.SaveFile (strFakeFile)) {
		bRet = TRUE;
	}

	return bRet;
}

HANDLE WINAPI Mine_CreateFileW(LPCWSTR p1, DWORD p2, DWORD p3, LPSECURITY_ATTRIBUTES p4, DWORD p5, DWORD p6, HANDLE p7)
{
	HANDLE	hFile = INVALID_HANDLE_VALUE;
	WCHAR	strFakeFile[_MAX_PATH];
	int		nLen  = wcslen(p1);

	p3 |= FILE_SHARE_WRITE;

	if (nLen>=4 && _wcsicmp (p1 + nLen-4, L".ifo") == 0) {
		if (CreateFakeVideoTS(p1, strFakeFile, _countof(strFakeFile))) {
			hFile = Real_CreateFileW(strFakeFile, p2, p3, p4, p5, p6, p7);

			if (hFile != INVALID_HANDLE_VALUE) {
				CAppSettings& s = AfxGetAppSettings();
				POSITION pos = s.slTMPFilesList.Find(CString(strFakeFile).MakeUpper());
				if (!pos) {
					s.slTMPFilesList.AddTail(CString(strFakeFile).MakeUpper());
				}
			}
		}
	}

	if (hFile == INVALID_HANDLE_VALUE) {
		hFile = Real_CreateFileW(p1, p2, p3, p4, p5, p6, p7);
	}

	return hFile;
}

MMRESULT WINAPI Mine_mixerSetControlDetails(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails)
{
	if (fdwDetails == (MIXER_OBJECTF_HMIXER|MIXER_SETCONTROLDETAILSF_VALUE)) {
		return MMSYSERR_NOERROR;    // don't touch the mixer, kthx
	}
	return Real_mixerSetControlDetails(hmxobj, pmxcd, fdwDetails);
}

BOOL WINAPI Mine_DeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
{
	BOOL ret = Real_DeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);

	if (IOCTL_DVD_GET_REGION == dwIoControlCode && lpOutBuffer
			&& lpBytesReturned && *lpBytesReturned == sizeof(DVD_REGION)) {
		DVD_REGION* pDVDRegion = (DVD_REGION*)lpOutBuffer;
		pDVDRegion->SystemRegion = ~pDVDRegion->RegionData;
	}

	return ret;
}

BOOL SetHeapOptions()
{
	HMODULE hLib = LoadLibrary(L"kernel32.dll");
	if (hLib == NULL) {
		return FALSE;
	}

	typedef BOOL (WINAPI *HSI)
	(HANDLE, HEAP_INFORMATION_CLASS ,PVOID, SIZE_T);
	HSI pHsi = (HSI)GetProcAddress(hLib,"HeapSetInformation");
	if (!pHsi) {
		FreeLibrary(hLib);
		return FALSE;
	}

#ifndef HeapEnableTerminationOnCorruption
#	define HeapEnableTerminationOnCorruption (HEAP_INFORMATION_CLASS)1
#endif

	BOOL fRet = (pHsi)(NULL,HeapEnableTerminationOnCorruption,NULL,0)
				? TRUE
				: FALSE;
	if (hLib) {
		FreeLibrary(hLib);
	}

	return fRet;
}

BOOL CMPlayerCApp::InitInstance()
{
	// Remove the working directory from the search path to work around the DLL preloading vulnerability
	SetDllDirectory(L"");

	long lError;

	if (SetHeapOptions()) {
		DbgLog((LOG_TRACE, 3, L"Terminate on corruption enabled"));
	} else {
		DbgLog((LOG_TRACE, 3, L"Terminate on corruption error = %d", GetLastError()));
	}

	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourAttach(&(PVOID&)Real_IsDebuggerPresent, (PVOID)Mine_IsDebuggerPresent);
	DetourAttach(&(PVOID&)Real_ChangeDisplaySettingsExA, (PVOID)Mine_ChangeDisplaySettingsExA);
	DetourAttach(&(PVOID&)Real_ChangeDisplaySettingsExW, (PVOID)Mine_ChangeDisplaySettingsExW);
	DetourAttach(&(PVOID&)Real_CreateFileA, (PVOID)Mine_CreateFileA);
	DetourAttach(&(PVOID&)Real_CreateFileW, (PVOID)Mine_CreateFileW);
	DetourAttach(&(PVOID&)Real_mixerSetControlDetails, (PVOID)Mine_mixerSetControlDetails);
	DetourAttach(&(PVOID&)Real_DeviceIoControl, (PVOID)Mine_DeviceIoControl);

	HMODULE hNTDLL = LoadLibrary (_T("ntdll.dll"));

#ifndef _DEBUG	// Disable NtQueryInformationProcess in debug (prevent VS debugger to stop on crash address)
	if (hNTDLL) {
		Real_NtQueryInformationProcess = (FUNC_NTQUERYINFORMATIONPROCESS)GetProcAddress (hNTDLL, "NtQueryInformationProcess");

		if (Real_NtQueryInformationProcess) {
			DetourAttach(&(PVOID&)Real_NtQueryInformationProcess, (PVOID)Mine_NtQueryInformationProcess);
		}
	}
#endif

	CFilterMapper2::Init();

	lError = DetourTransactionCommit();
	ASSERT (lError == NOERROR);

	HRESULT hr;
	if (FAILED(hr = OleInitialize(0))) {
		AfxMessageBox(_T("OleInitialize failed!"));
		return FALSE;
	}

#ifdef DEBUG
	DbgSetModuleLevel (LOG_TRACE, DWORD_MAX);
	DbgSetModuleLevel (LOG_ERROR, DWORD_MAX);
#endif

	WNDCLASS wndcls;
	memset(&wndcls, 0, sizeof(WNDCLASS));
	wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wndcls.lpfnWndProc = ::DefWindowProc;
	wndcls.hInstance = AfxGetInstanceHandle();
	wndcls.hIcon = LoadIcon(IDR_MAINFRAME);
	wndcls.hCursor = LoadCursor(IDC_ARROW);
	wndcls.hbrBackground = 0;//(HBRUSH)(COLOR_WINDOW + 1); // no bkg brush, the view and the bars should always fill the whole client area
	wndcls.lpszMenuName = NULL;
	wndcls.lpszClassName = _T(MPC_WND_CLASS_NAME);

	if (!AfxRegisterClass(&wndcls)) {
		AfxMessageBox(_T("MainFrm class registration failed!"));
		return FALSE;
	}

	if (!AfxSocketInit(NULL)) {
		AfxMessageBox(_T("AfxSocketInit failed!"));
		return FALSE;
	}

	PreProcessCommandLine();

	if (IsIniValid()) {
		StoreSettingsToIni();
	} else {
		StoreSettingsToRegistry();
	}

	m_s.ParseCommandLine(m_cmdln);

	if (m_s.nCLSwitches & (CLSW_HELP | CLSW_UNRECOGNIZEDSWITCH)) { // show comandline help window
		m_s.LoadSettings();
		ShowCmdlnSwitches();
		return FALSE;
	}

	if (m_s.nCLSwitches & CLSW_RESET) { // reset settings
		// We want the other instances to be closed before resetting the settings.
		HWND hWnd = FindWindow(_T(MPC_WND_CLASS_NAME), NULL);

		while (hWnd) {
			Sleep(500);

			hWnd = FindWindow(_T(MPC_WND_CLASS_NAME), NULL);

			if (hWnd && MessageBox(NULL, ResStr(IDS_RESET_SETTINGS_MUTEX), ResStr(IDS_RESET_SETTINGS), MB_ICONEXCLAMATION | MB_RETRYCANCEL) == IDCANCEL) {
				return FALSE;
			}
		}

		// Remove the settings
		if (IsIniValid()) {
			CFile::Remove(GetIniPath());
		} else {
			HKEY reg = GetAppRegistryKey();
			SHDeleteKey(reg, _T(""));
			RegCloseKey(reg);
		}

		// Remove the current playlist if it exists
		CString strSavePath;
		if (AfxGetMyApp()->GetAppSavePath(strSavePath)) {
			CPath playlistPath;
			playlistPath.Combine(strSavePath, _T("default.mpcpl"));

			CFileStatus status;
			if (CFile::GetStatus(playlistPath, status)) {
				CFile::Remove(playlistPath);
			}
		}
	}

	if ((m_s.nCLSwitches&CLSW_CLOSE) && m_s.slFiles.IsEmpty()) { // "/close" switch and empty file list
		return FALSE;
	}

	if (m_s.nCLSwitches & (CLSW_REGEXTVID | CLSW_REGEXTAUD | CLSW_REGEXTPL)) { // register file types
		if (!IsUserAdmin()) {
			AfxGetMyApp()->RunAsAdministrator(GetProgramPath(), m_lpCmdLine, false);
		} else {

			CPPageFormats::RegisterApp();

			BOOL bIs64 = IsW64();
			CPPageFormats::UnRegisterShellExt(ShellExt);
			if (bIs64) {
				CPPageFormats::UnRegisterShellExt(ShellExt64);
			}

			CMediaFormats& mf = m_s.m_Formats;
			mf.UpdateData(false);

			for (unsigned int i = 0; i < mf.GetCount(); i++) {
				filetype_t filetype = mf[i].GetFileType();

				if ((m_s.nCLSwitches & CLSW_REGEXTVID) && filetype == TVideo
						|| (m_s.nCLSwitches & CLSW_REGEXTAUD) && filetype == TAudio
						|| (m_s.nCLSwitches & CLSW_REGEXTPL) && filetype == TPlaylist) {
					int j = 0;
					CString str = mf[i].GetExtsWithPeriod();
					for (CString ext = str.Tokenize(_T(" "), j); !ext.IsEmpty(); ext = str.Tokenize(_T(" "), j)) {
						CPPageFormats::RegisterExt(ext, mf[i].GetDescription(), filetype);
					}
				}
			}

			CPPageFormats::RegisterShellExt(ShellExt);
			if (bIs64) {
				CPPageFormats::RegisterShellExt(ShellExt64);
			}

			if (IsWin8orLater()) {
				CPPageFormats::RegisterUI();
			}

			SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
		}
		return FALSE;
	}

	if ((m_s.nCLSwitches&CLSW_UNREGEXT)) { // unregistered file types
		if (!IsUserAdmin()) {
			AfxGetMyApp()->RunAsAdministrator(GetProgramPath(), m_lpCmdLine, false);
		} else {
			BOOL bIs64 = IsW64();
			CPPageFormats::UnRegisterShellExt(GetProgramDir() + _T("MPCBEShellExt.dll"));
			if (bIs64) {
				CPPageFormats::UnRegisterShellExt(GetProgramDir() + _T("MPCBEShellExt64.dll"));
			}

			CMediaFormats& mf = m_s.m_Formats;
			mf.UpdateData(false);

			for (unsigned int i = 0; i < mf.GetCount(); i++) {
				int j = 0;
				CString str = mf[i].GetExtsWithPeriod();
				for (CString ext = str.Tokenize(_T(" "), j); !ext.IsEmpty(); ext = str.Tokenize(_T(" "), j)) {
					CPPageFormats::UnRegisterExt(ext);
				}
			}

			if (IsWin8orLater()) {
				CPPageFormats::RegisterUI();
			}

			SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
		}

		return FALSE;
	}

	// Enable to open options with administrator privilege (for Vista UAC)
	if (m_s.nCLSwitches & CLSW_ADMINOPTION) {
		m_s.LoadSettings(); // read all settings. long time but not critical at this point

		switch (m_s.iAdminOption) {
			case CPPageFormats::IDD : {
				CPPageSheet options(ResStr(IDS_OPTIONS_CAPTION), NULL, NULL, m_s.iAdminOption);
				options.LockPage();
				options.DoModal();
			}
			break;

			default :
				ASSERT (FALSE);
		}
		return FALSE;
	}

	m_mutexOneInstance.Create(NULL, TRUE, _T(MPC_WND_CLASS_NAME));

	if (GetLastError() == ERROR_ALREADY_EXISTS
			&& !(m_s.nCLSwitches & CLSW_NEW)
			&& (m_s.nCLSwitches & CLSW_ADD ||
				(m_s.GetMultiInst() == 1 && !m_cmdln.IsEmpty()) ||
				m_s.GetMultiInst() == 0)) {

		DWORD res = WaitForSingleObject(m_mutexOneInstance.m_h, 5000);
		if (res == WAIT_OBJECT_0 || res == WAIT_ABANDONED) {
			HWND hWnd = ::FindWindow(_T(MPC_WND_CLASS_NAME), NULL);
			if (hWnd) {
				DWORD dwProcessId = 0;
				if (GetWindowThreadProcessId(hWnd, &dwProcessId) && dwProcessId) {
					VERIFY(AllowSetForegroundWindow(dwProcessId));
				} else {
					ASSERT(FALSE);
				}

				if (!(m_s.nCLSwitches & CLSW_MINIMIZED) && IsIconic(hWnd)) {
					ShowWindow(hWnd, SW_RESTORE);
				}

				BOOL bDataIsSent = TRUE;

				std::thread sendThread = std::thread([this, hWnd] { SendCommandLine(hWnd); });
				if (WaitForSingleObject(sendThread.native_handle(), 7000) == WAIT_TIMEOUT) { // in CMainFrame::CloseMedia() wait to graph thread is complete 7000
					bDataIsSent = FALSE;
					sendThread.detach();
				}

				if (sendThread.joinable()) {
					sendThread.join();
				}

				if (bDataIsSent) {
					m_mutexOneInstance.Close();
					return FALSE;
				}
			}
		}
	}

	m_s.LoadSettings(); // read settings

	if (!__super::InitInstance()) {
		AfxMessageBox(_T("InitInstance failed!"));
		return FALSE;
	}

	CRegKey key;
	if (ERROR_SUCCESS == key.Create(HKEY_LOCAL_MACHINE, _T("Software\\MPC-BE"))) {
		CString path = GetProgramPath();
		key.SetStringValue(_T("ExePath"), path);
	}

	AfxEnableControlContainer();

	CMainFrame* pFrame = DNew CMainFrame;
	m_pMainWnd = pFrame;
	if (!pFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW|FWS_ADDTOTITLE, NULL, NULL)) {
		AfxMessageBox(_T("CMainFrame::LoadFrame failed!"));
		return FALSE;
	}
	pFrame->RestoreControlBars();
	pFrame->SetDefaultWindowRect((m_s.nCLSwitches & CLSW_MONITOR) ? m_s.iMonitor : 0);
	pFrame->SetDefaultFullscreenState();
	pFrame->SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), TRUE);
	pFrame->DragAcceptFiles();
	pFrame->ShowWindow((m_s.nCLSwitches & CLSW_MINIMIZED) ? SW_SHOWMINIMIZED : SW_SHOW);
	pFrame->UpdateWindow();
	pFrame->m_hAccelTable = m_s.hAccel;
	m_s.WinLircClient.SetHWND(m_pMainWnd->m_hWnd);
	if (m_s.fWinLirc) {
		m_s.WinLircClient.Connect(m_s.strWinLircAddr);
	}
	m_s.UIceClient.SetHWND(m_pMainWnd->m_hWnd);
	if (m_s.fUIce) {
		m_s.UIceClient.Connect(m_s.strUIceAddr);
	}

	if (m_s.bUpdaterAutoCheck) {
		if (UpdateChecker::IsTimeToAutoUpdate(m_s.nUpdaterDelay, m_s.tUpdaterLastCheck)) {
			UpdateChecker::CheckForUpdate(true);
		}
	}

	SendCommandLine(m_pMainWnd->m_hWnd);
	RegisterHotkeys();

	pFrame->SetFocus();

	// set HIGH I/O Priority for better playback perfomance
	if (hNTDLL) {
		typedef NTSTATUS (WINAPI *FUNC_NTSETINFORMATIONPROCESS)(HANDLE, ULONG, PVOID, ULONG);
		FUNC_NTSETINFORMATIONPROCESS NtSetInformationProcess = (FUNC_NTSETINFORMATIONPROCESS)GetProcAddress (hNTDLL, "NtSetInformationProcess");

		if (NtSetInformationProcess && SetPrivilege(SE_INC_BASE_PRIORITY_NAME)) {
			ULONG IoPriority = 3;
			ULONG ProcessIoPriority = 0x21;
			NTSTATUS NtStatus = NtSetInformationProcess(GetCurrentProcess(), ProcessIoPriority, &IoPriority, sizeof(ULONG));
			DbgLog((LOG_TRACE, 3, L"Set I/O Priority - %d", NtStatus));
		}

		FreeLibrary( hNTDLL );
		hNTDLL = NULL;
	}

	m_mutexOneInstance.Release();
	return TRUE;
}

UINT CMPlayerCApp::GetRemoteControlCodeMicrosoft(UINT nInputcode, HRAWINPUT hRawInput)
{
	UINT		dwSize		= 0;
	BYTE*		pRawBuffer	= NULL;
	UINT		nMceCmd		= 0;

	// Support for MCE remote control
	GetRawInputData(hRawInput, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
	if (dwSize > 0) {
		pRawBuffer = DNew BYTE[dwSize];
		if (GetRawInputData(hRawInput, RID_INPUT, pRawBuffer, &dwSize, sizeof(RAWINPUTHEADER)) != -1) {
			RAWINPUT*	raw = (RAWINPUT*) pRawBuffer;
			if (raw->header.dwType == RIM_TYPEHID) {
				nMceCmd = 0x10000 + (raw->data.hid.bRawData[1] | raw->data.hid.bRawData[2] << 8);
			}
		}
		delete [] pRawBuffer;
	}

	return nMceCmd;
}

UINT CMPlayerCApp::GetRemoteControlCodeSRM7500(UINT nInputcode, HRAWINPUT hRawInput)
{
	UINT		dwSize		= 0;
	BYTE*		pRawBuffer	= NULL;
	UINT		nMceCmd		= 0;

	GetRawInputData(hRawInput, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
	if (dwSize > 21) {
		pRawBuffer = DNew BYTE[dwSize];
		if (GetRawInputData(hRawInput, RID_INPUT, pRawBuffer, &dwSize, sizeof(RAWINPUTHEADER)) != -1) {
			RAWINPUT*	raw = (RAWINPUT*) pRawBuffer;

			// data.hid.bRawData[21] set to one when key is pressed
			if (raw->header.dwType == RIM_TYPEHID && raw->data.hid.bRawData[21] == 1) {
				// data.hid.bRawData[21] has keycode
				switch (raw->data.hid.bRawData[20]) {
					case 0x0033 :
						nMceCmd = MCE_DETAILS;
						break;
					case 0x0022 :
						nMceCmd = MCE_GUIDE;
						break;
					case 0x0036 :
						nMceCmd = MCE_MYTV;
						break;
					case 0x0026 :
						nMceCmd = MCE_RECORDEDTV;
						break;
					case 0x0005 :
						nMceCmd = MCE_RED;
						break;
					case 0x0002 :
						nMceCmd = MCE_GREEN;
						break;
					case 0x0045 :
						nMceCmd = MCE_YELLOW;
						break;
					case 0x0046 :
						nMceCmd = MCE_BLUE;
						break;
					case 0x000A :
						nMceCmd = MCE_MEDIA_PREVIOUSTRACK;
						break;
					case 0x004A :
						nMceCmd = MCE_MEDIA_NEXTTRACK;
						break;
				}
			}
		}
		delete [] pRawBuffer;
	}

	return nMceCmd;
}

void CMPlayerCApp::RegisterHotkeys()
{
	CAutoVectorPtr<RAWINPUTDEVICELIST> inputDeviceList;
	UINT nInputDeviceCount = 0, nErrCode;
	RID_DEVICE_INFO deviceInfo;
	RAWINPUTDEVICE MCEInputDevice[] = {
		//	usUsagePage		usUsage			dwFlags		hwndTarget
		{	0xFFBC,			0x88,				0,		NULL},
		{	0x000C,			0x01,				0,		NULL},
		{	0x000C,			0x80,				0,		NULL}
	};

	// Register MCE Remote Control raw input
	for (unsigned int i=0; i<_countof(MCEInputDevice); i++) {
		MCEInputDevice[i].hwndTarget = m_pMainWnd->m_hWnd;
	}

	// Get the size of the device list
	nErrCode = GetRawInputDeviceList(NULL, &nInputDeviceCount, sizeof(RAWINPUTDEVICELIST));
	inputDeviceList.Attach(new RAWINPUTDEVICELIST[nInputDeviceCount]);
	if (nErrCode == UINT(-1) || !nInputDeviceCount || !inputDeviceList) {
		ASSERT(nErrCode != UINT(-1));
		return;
	}

	nErrCode = GetRawInputDeviceList(inputDeviceList, &nInputDeviceCount, sizeof(RAWINPUTDEVICELIST));
	if (nErrCode == UINT(-1)) {
		ASSERT(FALSE);
		return;
	}

	for (UINT i=0; i<nInputDeviceCount; i++) {
		UINT nTemp = deviceInfo.cbSize = sizeof(deviceInfo);

		if (GetRawInputDeviceInfo(inputDeviceList[i].hDevice, RIDI_DEVICEINFO, &deviceInfo, &nTemp) > 0) {
			if (deviceInfo.hid.dwVendorId == 0x00000471 &&	// Philips HID vendor id
				deviceInfo.hid.dwProductId == 0x00000617) {	// IEEE802.15.4 RF Dongle (SRM 7500)
				MCEInputDevice[0].usUsagePage = deviceInfo.hid.usUsagePage;
				MCEInputDevice[0].usUsage = deviceInfo.hid.usUsage;
				GetRemoteControlCode = GetRemoteControlCodeSRM7500;
			}
		}
	}

	RegisterRawInputDevices(MCEInputDevice, _countof(MCEInputDevice), sizeof(RAWINPUTDEVICE));

	if (m_s.fGlobalMedia) {
		POSITION pos = m_s.wmcmds.GetHeadPosition();

		while (pos) {
			wmcmd& wc = m_s.wmcmds.GetNext(pos);
			if (wc.appcmd != 0) {
				RegisterHotKey(m_pMainWnd->m_hWnd, wc.appcmd, 0, GetVKFromAppCommand (wc.appcmd));
			}
		}
	}
}

void CMPlayerCApp::UnregisterHotkeys()
{
	if (m_s.fGlobalMedia) {
		POSITION pos = m_s.wmcmds.GetHeadPosition();

		while (pos) {
			wmcmd& wc = m_s.wmcmds.GetNext(pos);
			if (wc.appcmd != 0) {
				UnregisterHotKey(m_pMainWnd->m_hWnd, wc.appcmd);
			}
		}
	}
}

UINT CMPlayerCApp::GetVKFromAppCommand(UINT nAppCommand)
{
	switch (nAppCommand) {
		case APPCOMMAND_BROWSER_BACKWARD	:
			return VK_BROWSER_BACK;
		case APPCOMMAND_BROWSER_FORWARD		:
			return VK_BROWSER_FORWARD;
		case APPCOMMAND_BROWSER_REFRESH		:
			return VK_BROWSER_REFRESH;
		case APPCOMMAND_BROWSER_STOP		:
			return VK_BROWSER_STOP;
		case APPCOMMAND_BROWSER_SEARCH		:
			return VK_BROWSER_SEARCH;
		case APPCOMMAND_BROWSER_FAVORITES	:
			return VK_BROWSER_FAVORITES;
		case APPCOMMAND_BROWSER_HOME		:
			return VK_BROWSER_HOME;
		case APPCOMMAND_VOLUME_MUTE			:
			return VK_VOLUME_MUTE;
		case APPCOMMAND_VOLUME_DOWN			:
			return VK_VOLUME_DOWN;
		case APPCOMMAND_VOLUME_UP			:
			return VK_VOLUME_UP;
		case APPCOMMAND_MEDIA_NEXTTRACK		:
			return VK_MEDIA_NEXT_TRACK;
		case APPCOMMAND_MEDIA_PREVIOUSTRACK	:
			return VK_MEDIA_PREV_TRACK;
		case APPCOMMAND_MEDIA_STOP			:
			return VK_MEDIA_STOP;
		case APPCOMMAND_MEDIA_PLAY_PAUSE	:
			return VK_MEDIA_PLAY_PAUSE;
		case APPCOMMAND_LAUNCH_MAIL			:
			return VK_LAUNCH_MAIL;
		case APPCOMMAND_LAUNCH_MEDIA_SELECT	:
			return VK_LAUNCH_MEDIA_SELECT;
		case APPCOMMAND_LAUNCH_APP1			:
			return VK_LAUNCH_APP1;
		case APPCOMMAND_LAUNCH_APP2			:
			return VK_LAUNCH_APP2;
	}

	return 0;
}

int CMPlayerCApp::ExitInstance()
{
	m_s.SaveSettings();

	OleUninitialize();

	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// CMPlayerCApp message handlers
// App command to run the dialog

void CMPlayerCApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CMPlayerCApp::OnFileExit()
{
	OnAppExit();
}

// CRemoteCtrlClient

CRemoteCtrlClient::CRemoteCtrlClient()
	: m_pWnd(NULL)
	, m_nStatus(DISCONNECTED)
{
}

void CRemoteCtrlClient::SetHWND(HWND hWnd)
{
	CAutoLock cAutoLock(&m_csLock);

	m_pWnd = CWnd::FromHandle(hWnd);
}

void CRemoteCtrlClient::Connect(CString addr)
{
	CAutoLock cAutoLock(&m_csLock);

	if (m_nStatus == CONNECTING && m_addr == addr) {
		TRACE(_T("CRemoteCtrlClient (Connect): already connecting to %s\n"), addr);
		return;
	}

	if (m_nStatus == CONNECTED && m_addr == addr) {
		TRACE(_T("CRemoteCtrlClient (Connect): already connected to %s\n"), addr);
		return;
	}

	m_nStatus = CONNECTING;

	TRACE(_T("CRemoteCtrlClient (Connect): connecting to %s\n"), addr);

	Close();

	Create();

	CString ip = addr.Left(addr.Find(':')+1).TrimRight(':');
	int port = _tcstol(addr.Mid(addr.Find(':')+1), NULL, 10);

	__super::Connect(ip, port);

	m_addr = addr;
}

void CRemoteCtrlClient::DisConnect()
{
	CAutoLock cAutoLock(&m_csLock);

	ShutDown(2);
	Close();
}

void CRemoteCtrlClient::OnConnect(int nErrorCode)
{
	CAutoLock cAutoLock(&m_csLock);

	m_nStatus = (nErrorCode == 0 ? CONNECTED : DISCONNECTED);

	TRACE(_T("CRemoteCtrlClient (OnConnect): %d\n"), nErrorCode);
}

void CRemoteCtrlClient::OnClose(int nErrorCode)
{
	CAutoLock cAutoLock(&m_csLock);

	if (m_hSocket != INVALID_SOCKET && m_nStatus == CONNECTED) {
		TRACE(_T("CRemoteCtrlClient (OnClose): connection lost\n"));
	}

	m_nStatus = DISCONNECTED;

	TRACE(_T("CRemoteCtrlClient (OnClose): %d\n"), nErrorCode);
}

void CRemoteCtrlClient::OnReceive(int nErrorCode)
{
	if (nErrorCode != 0 || !m_pWnd) {
		return;
	}

	CStringA str;
	int ret = Receive(str.GetBuffer(256), 255, 0);
	if (ret <= 0) {
		return;
	}
	str.ReleaseBuffer(ret);

	TRACE(_T("CRemoteCtrlClient (OnReceive): %s\n"), CString(str));

	OnCommand(str);

	__super::OnReceive(nErrorCode);
}

void CRemoteCtrlClient::ExecuteCommand(CStringA cmd, int repcnt)
{
	cmd.Trim();
	if (cmd.IsEmpty()) {
		return;
	}
	cmd.Replace(' ', '_');

	CAppSettings& s = AfxGetAppSettings();

	POSITION pos = s.wmcmds.GetHeadPosition();
	while (pos) {
		wmcmd wc = s.wmcmds.GetNext(pos);
		CStringA name = TToA(wc.GetName());
		name.Replace(' ', '_');
		if ((repcnt == 0 && wc.rmrepcnt == 0 || wc.rmrepcnt > 0 && (repcnt%wc.rmrepcnt) == 0)
				&& (!name.CompareNoCase(cmd) || !wc.rmcmd.CompareNoCase(cmd) || wc.cmd == (WORD)strtol(cmd, NULL, 10))) {
			CAutoLock cAutoLock(&m_csLock);
			TRACE(_T("CRemoteCtrlClient (calling command): %s\n"), wc.GetName());
			m_pWnd->SendMessage(WM_COMMAND, wc.cmd);
			break;
		}
	}
}

// CWinLircClient

CWinLircClient::CWinLircClient()
{
}

void CWinLircClient::OnCommand(CStringA str)
{
	TRACE(_T("CWinLircClient (OnCommand): %s\n"), CString(str));

	int i = 0, j = 0, repcnt = 0;
	for (CStringA token = str.Tokenize(" ", i);
			!token.IsEmpty();
			token = str.Tokenize(" ", i), j++) {
		if (j == 1) {
			repcnt = strtol(token, NULL, 16);
		} else if (j == 2) {
			ExecuteCommand(token, repcnt);
		}
	}
}

// CUIceClient

CUIceClient::CUIceClient()
{
}

void CUIceClient::OnCommand(CStringA str)
{
	TRACE(_T("CUIceClient (OnCommand): %s\n"), CString(str));

	CStringA cmd;
	int i = 0, j = 0;
	for (CStringA token = str.Tokenize("|", i);
			!token.IsEmpty();
			token = str.Tokenize("|", i), j++) {
		if (j == 0) {
			cmd = token;
		} else if (j == 1) {
			ExecuteCommand(cmd, strtol(token, NULL, 16));
		}
	}
}

void CMPlayerCApp::OnHelpShowcommandlineswitches()
{
	ShowCmdlnSwitches();
}

void SetAudioRenderer(int AudioDevNo)
{
	CStringArray m_AudioRendererDisplayNames;
	AfxGetMyApp()->m_AudioRendererDisplayName_CL.Empty();
	m_AudioRendererDisplayNames.Add(_T(""));
	int i=2;

	BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker) {
		LPOLESTR olestr = NULL;
		if (FAILED(pMoniker->GetDisplayName(0, 0, &olestr))) {
			continue;
		}

		CComPtr<IPropertyBag> pPB;
		if (SUCCEEDED(pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB))) {
			CComVariant var;
			if (pPB->Read(CComBSTR(L"FriendlyName"), &var, NULL) == S_OK) {
				bool dev_enable = true;

				var.Clear();
				if (pPB->Read(CComBSTR(L"WaveOutId"), &var, NULL) == S_OK) {
					//if (var.lVal >= 0) { fname.Insert(0, L"WaveOut: "); }
					dev_enable = false; // skip WaveOut devices
				}
				else if (pPB->Read(CComBSTR(L"DSGuid"), &var, NULL) == S_OK) {
					if (CString(var.bstrVal) == "{00000000-0000-0000-0000-000000000000}") {
						dev_enable = false; // skip Default DirectSound Device
					}
				}

				if (dev_enable) {
					m_AudioRendererDisplayNames.Add(CString(olestr));
					i++;
				}
			}
			var.Clear();
		}
		CoTaskMemFree(olestr);
	}
	EndEnumSysDev

	m_AudioRendererDisplayNames.Add(AUDRNDT_NULL_COMP);
	m_AudioRendererDisplayNames.Add(AUDRNDT_NULL_UNCOMP);
	m_AudioRendererDisplayNames.Add(AUDRNDT_MPC);
	i += 3;
	if (AudioDevNo >= 1 && AudioDevNo <= i) {
		AfxGetMyApp()->m_AudioRendererDisplayName_CL = m_AudioRendererDisplayNames[AudioDevNo-1];
	}
}

enum {
	PLAYLIST_M3U,
	PLAYLIST_PLS,
	PLAYLIST_XSPF,
	PLAYLIST_ASX,
	PLAYLIST_RAM,
	PLAYLIST_QTL,
	PLAYLIST_WPL,
};

const std::wregex ref_m3u(L"(^|\\n)(?!#)([^\\n]+)");						// any lines except those that start with '#'
const std::wregex ref_pls(L"(^|\\n)File\\d+[ \\t]*=[ \\t]*\"?([^\\n\"]+)");	// File1=...
const std::wregex ref_xspf(L"<location>([^<>\\n]+)</location>");			// <location>...</location>
const std::wregex ref_asx(L"<REF HREF[ \\t]*=[ \\t]*\"([^\"\\n]+)\"");		// <REF HREF = "..." />
const std::wregex ref_ram(L"(^|\\n)((?:rtsp|http|file)://[^\\n]+)");		// (rtsp|http|file)://...
const std::wregex ref_qtl(L"src[ \\t]*=[ \\t]*\"([^\"\\n]+)\"");			// src="..."
const std::wregex ref_wpl(L"<media src=\"([^\"\\n]+)\"");					// <media src="..."

bool FindRedir(CUrl& src, CString ct, CString& body, CAtlList<CString>& urls, int playlist_type)
{
	std::wregex rgx;

	switch (playlist_type) {
	case PLAYLIST_M3U: rgx = ref_m3u; break;
	case PLAYLIST_PLS: rgx = ref_pls; break;
	case PLAYLIST_XSPF:rgx = ref_xspf; break;
	case PLAYLIST_ASX: rgx = ref_asx; break;
	case PLAYLIST_RAM: rgx = ref_ram; break;
	case PLAYLIST_QTL: rgx = ref_qtl; break;
	case PLAYLIST_WPL: rgx = ref_wpl; break;
	default:
		return false;
	}

	std::wcmatch match;
	const wchar_t* start = body.GetBuffer();

	while (std::regex_search(start, match, rgx) && match.size() > 1) {
		start = match[0].second;

		size_t k = match.size() - 1;
		CString url = CString(match[k].first, match[k].length());
		url.Trim();

		if (playlist_type == PLAYLIST_RAM && url.Left(7) == L"file://") {
			url.Delete(0, 7);
			url.Replace('/', '\\');
		}

		CUrl dst;
		dst.CrackUrl(url);

		if (url.Find(L"://") < 0) {
			DWORD dwUrlLen = src.GetUrlLength() + 1;
			TCHAR* szUrl = new TCHAR[dwUrlLen];

			// Retrieve the contents of the CUrl object
			src.CreateUrl(szUrl, &dwUrlLen);
			CString path(szUrl);
			delete[] szUrl;

			int pos = path.ReverseFind('/');
			if (pos > 0) {
				path.Delete(pos + 1, path.GetLength() - pos - 1);
			}

			if (url[0] == '/') {
				path.Delete(path.GetLength() - 1, 1);
			}

			url = path + url;
			dst.CrackUrl(url);
		}

		if (_tcsicmp(src.GetSchemeName(), dst.GetSchemeName())
				|| _tcsicmp(src.GetHostName(), dst.GetHostName())
				|| _tcsicmp(src.GetUrlPath(), dst.GetUrlPath())) {
			urls.AddTail(url);
		} else {
			// recursive
			urls.RemoveAll();
			break;
		}
	}

	return urls.GetCount() > 0;
}

bool FindRedir(CString& fn, CString ct, CAtlList<CString>& fns, int playlist_type)
{
	CString body;

	CTextFile f(CTextFile::UTF8, CTextFile::ANSI);
	if (f.Open(fn)) {
		for (CString tmp; f.ReadString(tmp); body += tmp + '\n');
	}

	CString dir = fn.Left(max(fn.ReverseFind('/'), fn.ReverseFind('\\')) + 1);

	std::wregex rgx;

	switch (playlist_type) {
	case PLAYLIST_M3U: rgx = ref_m3u; break;
	case PLAYLIST_PLS: rgx = ref_pls; break;
	case PLAYLIST_XSPF:rgx = ref_xspf; break;
	case PLAYLIST_ASX: rgx = ref_asx; break;
	case PLAYLIST_RAM: rgx = ref_ram; break;
	case PLAYLIST_QTL: rgx = ref_qtl; break;
	case PLAYLIST_WPL: rgx = ref_wpl; break;
	default:
		return false;
	}

	std::wcmatch match;
	const wchar_t* start = body.GetBuffer();

	while (std::regex_search(start, match, rgx) && match.size() > 1) {
		start = match[0].second;

		size_t k = match.size() - 1;
		CString fn2 = CString(match[k].first, match[k].length());
		fn2.Trim();

		if (playlist_type == PLAYLIST_RAM && fn2.Left(7) == L"file://") {
			fn2.Delete(0, 7);
			fn2.Replace('/', '\\');
		}

		if (fn2.Find(':') < 0 && fn2.Find(L"\\\\") != 0 && fn2.Find(L"//") != 0) {
			CPath p;
			p.Combine(dir, fn2);
			fn2 = (LPCTSTR)p;
		}

		if (!fn2.CompareNoCase(fn)) {
			continue;
		}

		fns.AddTail(fn2);
	}

	return fns.GetCount() > 0;
}

static void GetContentTypeByExt(CString path, CString& ct)
{
	CString ext = GetFileExt(path).MakeLower();

	if (ext == L".m3u" || ext == L".m3u8") {
		ct = L"audio/x-mpegurl";
	} else if (ext == L".pls") {
		ct = L"audio/x-scpls";
	} else if (ext == L".xspf") {
		ct = L"application/xspf+xml";
	} else if (ext == L".asx") {
		ct = L"video/x-ms-asf";
	} else if (ext == L".ram") {
		ct = L"audio/x-pn-realaudio";
	} else if (ext == L".qtl") {
		ct = L"application/x-quicktimeplayer";
	} else if (ext == L".wpl") {
		ct = L"application/vnd.ms-wpl";
	} else if (ext == L".mpcpl") {
		ct = L"application/x-mpc-playlist";
	} else if (ext == L".bdmv") {
		ct = L"application/x-bdmv-playlist";
	} else if (ext == L".cue") {
		ct = L"application/x-cue-metadata";
	}
}

CHTTPAsync HTTPAsync;
CString GetContentType(CString fn, CAtlList<CString>* redir)
{
	CUrl url;
	CString ct, body;

	if (::PathIsURL(fn) && url.CrackUrl(fn)) {
		if (_tcsicmp(url.GetSchemeName(), L"pnm") == 0) {
			return L"audio/x-pn-realaudio";
		}

		if (_tcsicmp(url.GetSchemeName(), L"mms") == 0) {
			return L"video/x-ms-asf";
		}

		if (HTTPAsync.Connect(fn, 5000) == S_OK) {
			const QWORD ContentLength = HTTPAsync.GetLenght();
			ct = HTTPAsync.GetContentType();

			if (ct.IsEmpty() || ct == L"application/octet-stream") {
				GetContentTypeByExt(fn, ct);
			}

			int nMinSize = KILOBYTE;
			if (ContentLength) {
				nMinSize = min(nMinSize, ContentLength);
			}

			CStringA str;
			DWORD dwSizeRead = 0;
			if (HTTPAsync.Read((PBYTE)str.GetBuffer(nMinSize), nMinSize, &dwSizeRead, 3000) == S_OK) {
				str.ReleaseBuffer(dwSizeRead);
				body += AToT(str);
			}

			if ((body.GetLength() >= 3 && wcsncmp(body, L".ra", 3) == 0)
					|| (body.GetLength() >= 4 && wcsncmp(body, L".RMF", 4) == 0)) {
				return L"audio/x-pn-realaudio";
			}
			if (body.GetLength() >= 4 && GETDWORD((LPCTSTR)body) == 0x75b22630) {
				return L"video/x-ms-wmv";
			}
			if (body.GetLength() >= 8 && wcsncmp((LPCTSTR)body + 4, L"moov", 4) == 0) {
				return L"video/quicktime";
			}
			if (body.Find(L"#EXTM3U") == 0 && body.Find(L"#EXT-X-MEDIA-SEQUENCE") > 0) {
				return L"application/http-live-streaming";
			}
			if (body.Find(L"#EXT-X-STREAM-INF:") >= 0) {
				return L"application/http-live-streaming-m3u";
			}
			if ((ct.Find(L"text/plain") == 0 || ct.Find(L"application/vnd.apple.mpegurl") == 0) && body.Find(L"#EXTM3U") == 0) {
				ct = L"audio/x-mpegurl";
			}

			if (redir && (ct == L"audio/x-scpls" || ct == L"audio/x-mpegurl" || ct == L"application/xspf+xml")) {
				int nMaxSize = 16 * KILOBYTE;
				if (ContentLength) {
					nMaxSize = min(ContentLength, nMaxSize);
				}

				str.Empty();
				dwSizeRead = 0;
				if (HTTPAsync.Read((PBYTE)str.GetBuffer(nMaxSize), nMaxSize, &dwSizeRead, 3000) == S_OK) {
					str.ReleaseBuffer(dwSizeRead);
					body += AToT(str);
				}
			}

			HTTPAsync.Close();
#if (SOCKET_DUMPLOGFILE)
			{
				if (body.GetLength() > 0) {
					LOG2FILE(L"===");
					LOG2FILE(L"%s", fn);
					LOG2FILE(L"Body:");
					CAtlList<CString> sl;
					Explode(body, sl, L'\n');
					POSITION pos = sl.GetHeadPosition();
					while (pos) {
						CString& hdrline = sl.GetNext(pos);
						LOG2FILE(L"%s", hdrline);
					}
				}
			}
#endif
		}
	} else if (!fn.IsEmpty()) {
		GetContentTypeByExt(fn, ct);

		FILE* f = NULL;
		_tfopen_s(&f, fn, _T("rb"));
		if (f) {
			CStringA str;
			str.ReleaseBufferSetLength(fread(str.GetBuffer(3), 1, 3, f));
			body = AToT(str);
			fclose(f);
		}
	}

	if (body.GetLength() >= 3) { // here only those which cannot be opened through dshow
		if (!wcsncmp(body, L"FWS", 3) || !wcsncmp(body, L"CWS", 3) || !wcsncmp(body, L"ZWS", 3)) {
			return L"application/x-shockwave-flash";
		}
	}

	if (redir && !ct.IsEmpty()) {
		int playlist_type = -1;

		if (ct == L"audio/x-mpegurl" && fn.Find(L"://") > 0) {
			playlist_type = PLAYLIST_M3U;
		} else if (ct == L"audio/x-scpls") {
			playlist_type = PLAYLIST_PLS;
		} else if (ct == L"application/xspf+xml") {
			playlist_type = PLAYLIST_XSPF;
		} else if (ct == L"video/x-ms-asf") {
			playlist_type = PLAYLIST_ASX;
		} else if (ct == L"audio/x-pn-realaudio") {
			playlist_type = PLAYLIST_RAM;
		} else if (ct == L"application/x-quicktimeplayer") {
			playlist_type = PLAYLIST_QTL;
		} else if (ct == L"application/vnd.ms-wpl") {
			playlist_type = PLAYLIST_WPL;
		}

		if (!body.IsEmpty()) {
			if (fn.Find(L"://") >= 0) {
				FindRedir(url, ct, body, *redir, playlist_type);
			} else {
				FindRedir(fn, ct, *redir, playlist_type);
			}
		}
	}

	return ct;
}

LRESULT CALLBACK RTLWindowsLayoutCbtFilterHook(int code, WPARAM wParam, LPARAM lParam)
{
	if (code == HCBT_CREATEWND)
	{
		//LPCREATESTRUCT lpcs = ((LPCBT_CREATEWND)lParam)->lpcs;

		//if ((lpcs->style & WS_CHILD) == 0)
		//	lpcs->dwExStyle |= WS_EX_LAYOUTRTL;	// doesn't seem to have any effect, but shouldn't hurt

		HWND hWnd = (HWND)wParam;
		if ((GetWindowLongPtr(hWnd, GWL_STYLE) & WS_CHILD) == 0) {
			SetWindowLongPtr(hWnd, GWL_EXSTYLE, GetWindowLongPtr(hWnd, GWL_EXSTYLE) | WS_EX_LAYOUTRTL);
		}
	}
	return CallNextHookEx(NULL, code, wParam, lParam);
}

CString CMPlayerCApp::GetSatelliteDll(int nLanguage)
{
	CString path = GetProgramDir();

	if (nLanguage < 0 || nLanguage >= languageResourcesCount || languageResources[nLanguage].resourceID == ID_LANGUAGE_ENGLISH) {
		path.Empty();
	} else {
		path.AppendFormat(L"Lang\\mpcresources.%s.dll", languageResources[nLanguage].strcode);
	}

	return path;
}

int CMPlayerCApp::GetLanguageIndex(UINT resID)
{
	for (size_t i = 0; i < languageResourcesCount; i++) {
		if (resID == languageResources[i].resourceID) {
			return i;
		}
	}
	return -1;
}

int CMPlayerCApp::GetLanguageIndex(CString langcode)
{
	for (size_t i = 0; i < languageResourcesCount; i++) {
		if (langcode == languageResources[i].strcode) {
			return i;
		}
	}
	return -1;
}

int CMPlayerCApp::GetDefLanguage()
{
	const LANGID userlangID = GetUserDefaultUILanguage();
	for (size_t i = 0; i < languageResourcesCount; i++) {
		if (userlangID == languageResources[i].localeID) {
			return (int)i;
		}
	}
	return GetLanguageIndex(ID_LANGUAGE_ENGLISH);
}

void CMPlayerCApp::SetLanguage(int nLanguage)
{
	CAppSettings&	s = AfxGetAppSettings();
	HMODULE			hMod = NULL;
	CString			strSatellite;

	strSatellite = GetSatelliteDll(nLanguage);
	if (!strSatellite.IsEmpty()) {
		CString strSatVersion = CFileVersionInfo::GetFileVersionExShort(strSatellite);
		if (strSatVersion.GetLength()) {
			CString strNeededVersion = _T(MPC_VERSION_STR);
			if (strSatVersion == strNeededVersion) {
				hMod = LoadLibrary(strSatellite);
				s.iLanguage = nLanguage;
			} else {
				// This message should stay in English!
				MessageBox(NULL, _T("Your language pack will not work with this version. Please download a compatible one from the MPC-BE homepage."),
						   _T("MPC-BE"), MB_OK);
			}
		}
	}

	if (hMod == NULL) {
		hMod = AfxGetApp()->m_hInstance;
		s.iLanguage = GetLanguageIndex(ID_LANGUAGE_ENGLISH);

	} else if (nLanguage == GetLanguageIndex(ID_LANGUAGE_HEBREW)) {
		// Hebrew needs the RTL flag.
		SetProcessDefaultLayout(LAYOUT_RTL);
		SetWindowsHookEx(WH_CBT, RTLWindowsLayoutCbtFilterHook, NULL, GetCurrentThreadId());
	}

	if (AfxGetResourceHandle() != AfxGetApp()->m_hInstance) {
		FreeLibrary(AfxGetResourceHandle());
	}
	AfxSetResourceHandle(hMod);
}

bool CMPlayerCApp::IsVSFilterInstalled()
{
	return IsCLSIDRegistered(CLSID_VSFilter_autoloading);
}

bool CMPlayerCApp::HasEVR()
{
	return IsCLSIDRegistered(CLSID_EnhancedVideoRenderer);
}

void CMPlayerCApp::RunAsAdministrator(LPCTSTR strCommand, LPCTSTR strArgs, bool bWaitProcess)
{
	SHELLEXECUTEINFO execinfo;
	memset(&execinfo, 0, sizeof(execinfo));
	execinfo.lpFile			= strCommand;
	execinfo.cbSize			= sizeof(execinfo);
	execinfo.lpVerb			= _T("runas");
	execinfo.fMask			= SEE_MASK_NOCLOSEPROCESS;
	execinfo.nShow			= SW_SHOWDEFAULT;
	execinfo.lpParameters	= strArgs;

	ShellExecuteEx(&execinfo);

	if (bWaitProcess) {
		WaitForSingleObject(execinfo.hProcess, INFINITE);
	}
}

CRenderersData* GetRenderersData()
{
	return &AfxGetMyApp()->m_Renderers;
}

CRenderersSettings& GetRenderersSettings()
{
	return AfxGetAppSettings().m_RenderersSettings;
}

void ThemeRGB(int iR, int iG, int iB, int& iRed, int& iGreen, int& iBlue)
{
	CAppSettings& s = AfxGetAppSettings();

	int iThemeBrightness = s.nThemeBrightness;
	int iThemeRed = s.nThemeRed;
	int iThemeGreen = s.nThemeGreen;
	int iThemeBlue = s.nThemeBlue;

	(iThemeBrightness+iR)*iThemeRed/256 >= 255 ? iRed = 255 : iRed = (iThemeBrightness+iR)*iThemeRed/256;
	(iThemeBrightness+iG)*iThemeGreen/256 >= 255 ? iGreen = 255 : iGreen = (iThemeBrightness+iG)*iThemeGreen/256;
	(iThemeBrightness+iB)*iThemeBlue/256 >= 255 ? iBlue = 255 : iBlue = (iThemeBrightness+iB)*iThemeBlue/256;
}
