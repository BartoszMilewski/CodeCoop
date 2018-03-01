#if !defined (FOCUS_H)
#define FOCUS_H
//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

namespace Focus
{
	// The implementor of focus changes
	// Should ask Focus::Rin::GetFocusId and make its own adjustment
	// e.g. set focus to child window, or set other visual indications

	class Sink
	{
	public:
		virtual void OnFocusSwitch () = 0;
		virtual void OnClose (unsigned id) = 0;
	};

	// A ring of child windows, only one of them having keyboard focus at a time
	// The focus can be shifted forward, backward, or given to a particular child.
	// (This is usually done in response to tab and mouse activate)
	// Each focus change results in the notification sent to the current Focus::Sink

	class Ring
	{
	public:
		static const unsigned invalidId = -1;

	public:
		Ring ()
			: _sink (0),
			  _focusId (invalidId)
		{}

		void SetSink (Focus::Sink * sink) { _sink = sink; }
		void SwitchToNext ();
		void SwitchToPrevious ();
		void SwitchToThis (unsigned id);

		void ClearIds ();
		void AddId (unsigned id);
		void RemoveId (unsigned id);
		bool HasId (unsigned id) const;

		void HideThis (unsigned id);

		unsigned GetFocusId () const { return _focusId; }

	private:
		typedef std::vector<unsigned>::const_iterator ConstIdIter;
		typedef std::vector<unsigned>::iterator IdIter;

	private:
		Focus::Sink *			_sink;
		unsigned				_focusId;
		std::vector<unsigned>	_ids;
	};
}

#endif
