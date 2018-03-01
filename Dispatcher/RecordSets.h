#if !defined (RECORDSETS_H)
#define RECORDSETS_H
//--------------------------------
// (c) Reliable Software 1998-2006
// -------------------------------

#include "RecordSet.h"
#include "Mailbox.h" // Mailbox::Status enumeration
#include "ScriptStatus.h"

class Table;

class UnameRecordSet : public RecordSet
{
public:
	void GetBookmark (unsigned int row, Bookmark & bookmark) const
	{
		bookmark.SetName (GetName (row));
	}
	int GetRow (Bookmark const & bookmark) const;
	std::string const & GetName (unsigned int row) const;
	int CmpRows (unsigned int row1, unsigned int row2, int col) const { return 0; }
    unsigned int GetRowCount () const { return _names.size (); }
};

class PublicInboxRecordSet : public UnameRecordSet
{
public:
    PublicInboxRecordSet (Table & t);

    unsigned int GetColCount () const;
    char const * GetColumnHeading (unsigned int col) const;
    std::string GetFieldString (unsigned int row, unsigned int col) const;
	int CmpRows (unsigned int row1, unsigned int row2, int col) const;
    
    void GetImage (unsigned int item, int & imageId, int & overlay) const;
    void GetItemIcons (std::vector<int> & iconResIds) const;

private:
	std::vector<std::string>	_comments;
	std::vector<ScriptStatus>	_statuses;

    static char const * _columnHeadings [];
};

class MemberRecordSet : public UnameRecordSet
{
public:
    MemberRecordSet (Table & t);

    unsigned int GetColCount () const;
    char const * GetColumnHeading (unsigned int col) const;
    std::string GetFieldString (unsigned int row, unsigned int col) const;
	int CmpRows (unsigned int row1, unsigned int row2, int col) const;
    
    void GetImage (unsigned int item, int & imageId, int & overlay) const;
    void GetItemIcons (std::vector<int> & iconResIds) const;

private:
    static char const * _columnHeadings [];
};

class ProjectMemberRecordSet : public RecordSet
{
public:
    ProjectMemberRecordSet (Table & t, Restriction const & restrict);

	void GetBookmark (unsigned int row, Bookmark & bookmark) const
	{
		bookmark.SetKey (GetKey (row));
	}
	int GetRow (Bookmark const & bookmark) const;
	TripleKey const & GetKey (unsigned int row) const;
	int CmpRows (unsigned int row1, unsigned int row2, int col) const;

    unsigned int GetRowCount () const { return _keys.size (); }
    unsigned int GetColCount () const;
    char const * GetColumnHeading (unsigned int col) const;
    std::string GetFieldString (unsigned int row, unsigned int col) const;
    
    void GetImage (unsigned int item, int & imageId, int & overlay) const;
    void GetItemIcons (std::vector<int> & iconResIds) const;

private:
	std::string const			_projectName;	
	std::vector<TripleKey>		_keys;
	std::vector<std::string>	_userIds;
	std::vector<int>			_intUserIds;
	std::vector<std::string>	_paths;

    static char const * _columnHeadings [];
	static int const    _itemIconIds [];
    enum
    {
        iUser,
        iExUser,
        iSatUser,
        iExSatUser
    };

	static std::string const _stActive;
	static std::string const _stRemoved;
	static std::string const _locLocal;
	static std::string const _locCluster;
};

class RemoteHubRecordSet : public UnameRecordSet
{
public:
    RemoteHubRecordSet (Table & t);

	int CmpRows (unsigned int row1, unsigned int row2, int col) const;

    unsigned int GetColCount () const;
    char const * GetColumnHeading (unsigned int col) const;
    std::string GetFieldString (unsigned int row, unsigned int col) const;
    
    void GetImage (unsigned int item, int & imageId, int & overlay) const;
    void GetItemIcons (std::vector<int> & iconResIds) const;

private:
	std::vector<std::string>	_routes;
	std::vector<int>			_methods;

    static char const * _columnHeadings [];
	static int const    _itemIconIds [];
	static char const * _methodStr [];
};

class QuarantineRecordSet : public UnameRecordSet
{
public:
	QuarantineRecordSet (Table & t);

	int CmpRows (unsigned int row1, unsigned int row2, int col) const;

	unsigned int GetColCount () const;
	char const * GetColumnHeading (unsigned int col) const;
	std::string GetFieldString (unsigned int row, unsigned int col) const;

	void GetImage (unsigned int item, int & imageId, int & overlay) const;
	void GetItemIcons (std::vector<int> & iconResIds) const;

private:
	std::vector<std::string>	_statuses;

	static char const * _columnHeadings [];
};

class AlertLogRecordSet : public RecordSet
{
public:
	AlertLogRecordSet (Table & t);

	int CmpRows (unsigned int row1, unsigned int row2, int col) const;

	void GetBookmark (unsigned int row, Bookmark & bookmark) const
	{
		bookmark.SetId (_ids [row]);
	}
	int GetRow (Bookmark const & bookmark) const;

	unsigned int GetRowCount () const { return _ids.size (); }
	unsigned int GetColCount () const;
	char const * GetColumnHeading (unsigned int col) const;
	std::string GetFieldString (unsigned int row, unsigned int col) const;

	void GetImage (unsigned int item, int & imageId, int & overlay) const;
	void GetItemIcons (std::vector<int> & iconResIds) const;

private:
	std::vector<std::string>	_dates;
	std::vector<int>			_counts;

	static char const * _columnHeadings [];
};

#endif
