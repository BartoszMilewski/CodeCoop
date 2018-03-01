//-------------------------------------------
//  DatePicker.h
//  (c) Reliable Software, 2001
//-------------------------------------------
#include <WinLibBase.h>
#include "DatePicker.h"

namespace Win
{
	void DatePicker::SetNoDate()
	{
		SendMsg(DTM_SETFORMAT, 0, (LPARAM) "' no date'");

		//	We set the control's date to today so that if the user toggles it
		//	they'll get today's date instead of whatever it was
		SYSTEMTIME st;
		::GetLocalTime(&st);
		SendMsg(DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &st);

		//	clears the check box
		SendMsg(DTM_SETSYSTEMTIME, GDT_NONE);
	}

	void DatePicker::SetDate(unsigned year, unsigned month, unsigned day)
	{
		SYSTEMTIME st;
		memset(&st, 0, sizeof(st));
		st.wYear = year;
		st.wMonth = month;
		st.wDay = day;
		SendMsg(DTM_SETFORMAT);	//	set to default format
		SendMsg(DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &st);
	}

	bool DatePicker::Handler::OnNotify(NMHDR *hdr, long &result) throw (Win::Exception)
	{
		switch (hdr->code)
		{
		case DTN_DATETIMECHANGE:
			{
				Win::DatePicker dtp;
				dtp.Reset(hdr->hwndFrom);
				NMDATETIMECHANGE *pdtc = reinterpret_cast<NMDATETIMECHANGE *> (hdr);
				if (pdtc->dwFlags == GDT_NONE)
				{
					dtp.SetNoDate();
					OnChangeNoDate();
				}
				else
				{
					dtp.SendMsg(DTM_SETFORMAT);	//	set to default format
					OnChangeDate(pdtc->st.wYear, pdtc->st.wMonth, pdtc->st.wDay);
				}
				result = TRUE;
				return true;
			}
#if 0
		//	enable this code as needed for diagnosis
		case DTN_DROPDOWN:
			dbg << "DTN_DROPDOWN" << std::endl;
			return true;
		case DTN_CLOSEUP:
			dbg << "DTN_CLOSEUP" << std::endl;
			return true;
#endif
		}

		return false;
	}
}