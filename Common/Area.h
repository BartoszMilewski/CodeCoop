#if !defined (AREA_H)
#define AREA_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

namespace Area
{
    enum Location
    {
        Project,        // Project Area
        Original,       // Original Area *.og1 or *.og2
        Reference,      // Reference Area *.ref
        Synch,          // Synch Area *.syn
        PreSynch,		// Project PreSynch Area used during script unpack *.bak
        Staging,		// Project Staging Area *.prj
        OriginalBackup, // OriginalBackupArea *.ogx where x is always previous original id
		Temporary,		// Temporary *.tmp -- reconstructed files
		Compare,		// Compare *.cmp -- reconstructed files when comparing two project versions
		LocalEdits,		// Copy of local edits *.out
		LastArea
    };

	class Seq
	{
	public:
		Seq ()
			: _cur (0)
		{}
		bool AtEnd () const  { return _cur == LastArea; }
		void Advance ()      { _cur++; }

		Location GetArea () const { return static_cast<Location>(_cur); }

	private:
		int	_cur;
	};

	extern char const * Name [];
};

#endif
