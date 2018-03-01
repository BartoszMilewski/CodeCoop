#if !defined (PROGRESSDIALOGDATA_H)
#define PROGRESSDIALOGDATA_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include <Sys/Synchro.h>

namespace Progress
{
	class MeterDialogData
	{
	public:
		MeterDialogData (std::string const & title, unsigned initialDelay)
			: _title (title),
			  _initialDelay (initialDelay)
		{}

		std::string const & GetTitle () const { return _title; }

		void SetInitialDelay (unsigned int delay) { _initialDelay = delay; }
		unsigned int GetInitialDelay () const { return _initialDelay; }

		void SetCaption (std::string const & caption) { _caption = caption; }
		std::string const & GetCaption () const { return _caption; }

	private:
		std::string			_title;
		std::string			_caption;
		unsigned int		_initialDelay;
	};
}

#endif
