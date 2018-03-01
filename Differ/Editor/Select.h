#if !defined (SELECT_H)
#define SELECT_H
//
// (c) Reliable Software, 1997, 98, 99, 2000
//
#include "DocPosition.h"
#include <Win/Utility.h>

const int PARA_START = 0;
const int PARA_END = -1;

class SelectionNotificationSink;
namespace Selection
{
	class Dynamic
	{
		friend class Marker;

	public:
		Dynamic () 
			: _startParaNo (0),
			_endParaNo (0),
			_endCol (0),
	  		_paraSelection (false)		  
		{}
		bool IsSelected (int paraNo, int & startCol, int & endCol) const;
		bool IsSelected (int paraNo, int paraOffsetBegin, int paraOffsetEnd , int & offsetSelectBegin, int & offsetSelectEnd) const;
		bool IsSelected (int paraNo, int col) const;
		bool IsEmpty () const
		{
			if (_startParaNo == _endParaNo)
			{
				// begining and end in the same line
				return _anchor.IsEmpty () && _endCol == _anchor.Start (); 
			}
			return false;
		}
		void Select (Marker const & selMarker);
		bool IsParaSelection () const { return _paraSelection; }
		void StartSelect (int paraNo, int col, Win::KeyState flags);
		void StartSelectLine (int paraNo, int colBegin, int colEnd, Win::KeyState flags); 
		void ContinueSelect (int paraNo, int col);
		void ContinueSelect (int paraNo, int col, int prevParaNo, int prevCol);
		void ContinueSelectLine (int paraNo, int colStart, int colEnd);
		void EndSelect (int paraNo, int & col);
		bool EndSelectLine (int paraNo, int colBegin, int & colEnd);
		void CreateSelection (int paraNo, int paraBeginOffset, int paraEndOffset);
		void Clear ();	
		void SetSelectionNotificationSink (SelectionNotificationSink * sink) { _selSink = sink; }
	private:
		class Segment
		{
		public :
			Segment (int startCol = 0, int endCol = 0)
				: _startCol (startCol), _endCol (endCol)
			{}
			void Init (int startCol, int endCol);
			bool IsInSegment (int col) const;
			void Shrink ();
			int Start () const { return _startCol; }
			int End () const { return _endCol; }	
			void Union (int & startCol, int & endCol) const;
			bool IsEmpty () const;
			int AdjustStartCol (int prevCol) const;
		private :
			
			int _startCol;
			int _endCol;
		};
		bool IsSelected (int paraNo) const;	

		bool				_paraSelection;
		int                 _startParaNo;
		int                 _endParaNo;
		int                 _endCol;
		Segment             _anchor;

		SelectionNotificationSink * _selSink;
	};

	class Marker
	{
	public:
		Marker (Dynamic const & selection);
		Marker (DocPosition const & selectionBegin, DocPosition const & selectionEnd)
			:_selectionBegin (selectionBegin), _selectionEnd (selectionEnd)
		{}
		Marker () {}
		bool IsValid () const { return _selectionBegin.IsValid (); }
		bool IsSegment () const { return IsValid () && StartParaNo () == EndParaNo (); }
		bool IsEqual (ParaSegment const & segment) const;
		ParaSegment GetParaSegment  () const;
		DocPosition  Start () const { return _selectionBegin; }
		DocPosition  End () const { return _selectionEnd; }
		int StartParaNo () const { return _selectionBegin.ParaNo (); }
		int StartOffset () const { return _selectionBegin.ParaOffset (); }
		int EndParaNo () const { return _selectionEnd.ParaNo (); }
		int EndOffset () const { return _selectionEnd.ParaOffset (); }
		
	private:
		DocPosition _selectionBegin;
		DocPosition _selectionEnd;
	};
};

#endif
