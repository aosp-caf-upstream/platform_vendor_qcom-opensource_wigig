/*
 * Copyright (c) 2017, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "OsDebugLogger.h"

class WLCT_API CDebugLogger : public COsDebugLogger
{
public:
	struct WLCT_API S_LOG_CONTEXT
	{
		const TCHAR *funcName;
		const TCHAR *fileName;
		DWORD       line;
	};

	CDebugLogger(const TCHAR *tag, bool logToConsole = false);
	~CDebugLogger(void);
	COsDebugLogger::E_LOG_LEVEL GetTraceLevel() const
	{
		return m_eTraceLevel;
	}

	void LogDebugMessage(COsDebugLogger::E_LOG_LEVEL eTraceLevel,
	S_LOG_CONTEXT *pCtxt, const TCHAR *tmt, ...);
	void LogDebugMessageV(COsDebugLogger::E_LOG_LEVEL eTraceLevel,
	S_LOG_CONTEXT *pCtxt, const TCHAR *fmt, va_list args);

	static DWORD GetGlobalTraceLevel(void);
	static void LogGlobalDebugMessage(COsDebugLogger::E_LOG_LEVEL eTraceLevel,
	S_LOG_CONTEXT *pCtxt, const TCHAR *fmt, ...);

private:
	void ReadTraceSettings(const TCHAR *fileName);
	void SetDefaultSettings();

private:
#ifdef _WINDOWS
#pragma warning( disable : 4251 ) //suppress warning - it is OK on private members
#endif
	tstring m_sTag;
	bool    m_bLogToFile;
#ifdef _WINDOWS
#pragma warning( disable : 4251 ) //suppress warning - it is OK on private members
#endif
	tstring m_sLogFilePath;
#ifdef _WINDOWS
	HANDLE	m_hThread;
	bool	m_bExit;
	HANDLE	hNotifyEvent;

	friend void ReadSettingsThread(void *pLogger);
#endif
};

extern CDebugLogger *g_pLogger;
