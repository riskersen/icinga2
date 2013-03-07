/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "i2-base.h"
#if HAVE_BACKTRACE_SYMBOLS
#	include <execinfo.h>
#endif /* HAVE_BACKTRACE_SYMBOLS */

using namespace icinga;

boost::once_flag StackTrace::m_OnceFlag = BOOST_ONCE_INIT;

#ifdef _MSC_VER
#	pragma optimize("", off)
#endif /* _MSC_VER */

StackTrace::StackTrace(void)
{
	boost::call_once(m_OnceFlag, &StackTrace::Initialize);

#if HAVE_BACKTRACE_SYMBOLS
	m_Count = backtrace(m_Frames, sizeof(m_Frames) / sizeof(m_Frames[0]));
#else /* HAVE_BACKTRACE_SYMBOLS */
#	ifdef _WIN32
	m_Count = CaptureStackBackTrace(0, sizeof(m_Frames) / sizeof(m_Frames), m_Frames, NULL);
#	else /* _WIN32 */
	m_Count = 0;
#	endif /* _WIN32 */
#endif /* HAVE_BACKTRACE_SYMBOLS */
}

#ifdef _MSC_VER
#	pragma optimize("", on)
#endif /* _MSC_VER */

#ifdef _WIN32
StackTrace::StackTrace(PEXCEPTION_POINTERS exi)
{
	boost::call_once(m_OnceFlag, &StackTrace::Initialize);

	STACKFRAME64 frame;
	int architecture;

#ifdef _WIN64
	architecture = IMAGE_FILE_MACHINE_AMD64;

	frame.AddrPC.Offset = exi->ContextRecord->Rip;
	frame.AddrFrame.Offset = exi->ContextRecord->Rbp;
	frame.AddrStack.Offset = exi->ContextRecord->Rsp;
#else /* _WIN64 */
	architecture = IMAGE_FILE_MACHINE_I386;

	frame.AddrPC.Offset = exi->ContextRecord->Eip;
	frame.AddrFrame.Offset = exi->ContextRecord->Ebp;
	frame.AddrStack.Offset = exi->ContextRecord->Esp;
#endif  /* _WIN64 */

	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Mode = AddrModeFlat;

	m_Count = 0;

	while (StackWalk64(architecture, GetCurrentProcess(), GetCurrentThread(),
	    &frame, exi->ContextRecord, NULL, &SymFunctionTableAccess64,
	    &SymGetModuleBase64, NULL) && m_Count < sizeof(m_Frames) / sizeof(m_Frames[0])) {
		m_Frames[m_Count] = reinterpret_cast<void *>(frame.AddrPC.Offset);
		m_Count++;
	}
}
#endif /* _WIN32 */

void StackTrace::Initialize(void)
{
#ifdef _WIN32
	(void) SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
	(void) SymInitialize(GetCurrentProcess(), NULL, TRUE);
#endif /* _WIN32 */
}

/**
 * Prints a stacktrace to the specified stream.
 *
 * @param fp The stream.
 * @param ignoreFrames The number of stackframes to ignore (in addition to
 *		       the one this function is executing in).
 * @returns true if the stacktrace was printed, false otherwise.
 */
void StackTrace::Print(ostream& fp, int ignoreFrames) const
{
	fp << std::endl << "Stacktrace:" << std::endl;

#ifndef _WIN32
#	if HAVE_BACKTRACE_SYMBOLS
	char **messages = backtrace_symbols(m_Frames, m_Count);

	for (int i = ignoreFrames + 1; i < m_Count && messages != NULL; ++i) {
		String message = messages[i];

		char *sym_begin = strchr(messages[i], '(');

		if (sym_begin != NULL) {
			char *sym_end = strchr(sym_begin, '+');

			if (sym_end != NULL) {
				String sym = String(sym_begin + 1, sym_end);
				String sym_demangled = Utility::DemangleSymbolName(sym);

				if (sym_demangled.IsEmpty())
					sym_demangled = "<unknown function>";

				message = String(messages[i], sym_begin) + ": " + sym_demangled + " (" + String(sym_end);
			}
		}

        	fp << "\t(" << i - ignoreFrames - 1 << ") " << message << std::endl;
	}

	free(messages);

	fp << std::endl;
#	else /* HAVE_BACKTRACE_SYMBOLS */
	fp << "(not available)" << std::endl;
#	endif /* HAVE_BACKTRACE_SYMBOLS */
#else /* _WIN32 */
	for (int i = ignoreFrames + 1; i < m_Count; i++) {
		char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
		PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen = MAX_SYM_NAME;

		DWORD64 dwAddress = (DWORD64)m_Frames[i];
		DWORD dwDisplacement;
		DWORD64 dwDisplacement64;

		IMAGEHLP_LINE64 line;
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

		fp << "\t(" << i - ignoreFrames - 1 << ") ";
		
		if (SymGetLineFromAddr64(GetCurrentProcess(), dwAddress, &dwDisplacement, &line))
			fp << line.FileName << ":" << line.LineNumber;
		else
			fp << "(unknown file/line)";

		fp << ": ";

		if (SymFromAddr(GetCurrentProcess(), dwAddress, &dwDisplacement64, pSymbol))
			fp << pSymbol->Name << "+" << dwDisplacement64;
		else
			fp << "(unknown function)";

		 fp << std::endl;
	}
#endif /* _WIN32 */
}

ostream& icinga::operator<<(ostream& stream, const StackTrace& trace)
{
	trace.Print(stream, 1);
}

