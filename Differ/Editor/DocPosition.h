//
// Reliable Software (c) 2002
//
#if !defined (DOC_POSITION_H)
#define DOC_POSITION_H

class DocPosition
{
public:
	DocPosition (int paraNo, int paraOffset)
		: _paraNo (paraNo), _paraOffset (paraOffset)
	{}
	DocPosition ()
		:_paraNo (-1), _paraOffset (-1)
	{}
	bool IsValid () const {return _paraNo != -1; }
	bool operator ==(DocPosition const & other) const
	{
		return _paraNo == other.ParaNo () && _paraOffset == other.ParaOffset (); 
	}
	int ParaNo () const { return _paraNo; }
	void SetParaNo (int paraNo) { _paraNo = paraNo; }
	void DecraseParaNo (int k = 1) {_paraNo -= k; } 
	void IncreaseParaNo (int k = 1) {_paraNo += k; }
	int ParaOffset () const { return _paraOffset; }
	void DecraseParaOffset (int k = 1) {_paraOffset >=k ?_paraOffset -= k : _paraOffset = 0; }
	void IncraseParaOffset (int k = 1) {_paraOffset += k; }
	void SetParaOffset (int paraOffset) { _paraOffset = paraOffset; }
private:
	int _paraNo;
	int _paraOffset;
};

namespace Selection
{
	class Marker;
	class ParaSegment
	{
	public :
		ParaSegment (int paraNo, int paraOffset, int len = 0)
			: _docPosition (paraNo, paraOffset), _len (len)
		{}
		ParaSegment (DocPosition const & docPos, int len = 0)
			: _docPosition (docPos), _len (len)
		{}
		int ParaOffset () const { return _docPosition.ParaOffset (); }
		int ParaNo () const { return _docPosition.ParaNo (); }
		DocPosition  GetDocPosition () const { return _docPosition; } 
		int Len () const { return _len; }
		int End () const { return _docPosition.ParaOffset () + _len; }
		void SetLen (int len) { _len = len; }
		void SetParaOffset (int paraOffset) { _docPosition.SetParaOffset (paraOffset); }
		void SetParaNo (int paraNo) { _docPosition.SetParaNo (paraNo); }
	private :
		DocPosition _docPosition; 
		int			_len;
	};
};

#endif
