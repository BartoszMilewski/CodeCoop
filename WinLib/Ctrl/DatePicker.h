#if !defined (DATEPICKER_H)
#define DATEPICKER_H
//-------------------------------------------
//  DatePicker.h
//  (c) Reliable Software, 2001
//-------------------------------------------

#include "Controls.h"
#include <Win/Notification.h>

namespace Win
{
	class DatePicker : public SimpleControl
	{
	public:
		class Style: public Win::Style
		{
		public:
			DatePicker::Style & operator<<(void (DatePicker::Style::*method)())
			{
				(this->*method)();
				return *this;
			}
			void AllowNoDate()
			{
				_style |= DTS_SHOWNONE;
			}

			void SetShortDateFormat()
			{
				_style |= DTS_SHORTDATEFORMAT;
			}
		};
	public:
		//	if you use SetNoDate, you'll get a cleaner UI if you implement a
		//	notification handler that derives from DatePicker::Handler.
		//	The Handler::OnNotify not only unpacks the DTN_ messages, it also
		//	sets the display formats nicely.

		void SetNoDate();
		void SetDate(unsigned year, unsigned month, unsigned day);

		class Handler : public Notify::Handler
		{
			bool OnNotify (NMHDR * hdr, long & result) throw (Win::Exception);
			virtual void OnChangeDate(unsigned year, unsigned month, unsigned day) {}
			virtual void OnChangeNoDate() {}
		};
	};

	class DatePickerMaker : public ControlMaker
	{
	public:
		DatePickerMaker(Win::Dow::Handle winParent, int id)
			: ControlMaker(DATETIMEPICK_CLASS, winParent, id)
		{
			CommonControlsRegistry::Instance()->Add(CommonControlsRegistry::DATE);
		}
		DatePicker::Style & Style () { return static_cast<DatePicker::Style &> (_style); }
	};
}

#endif