#if !defined (FILEPROPS_H)
#define FILEPROPS_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "GlobalId.h"

class FileData;
namespace Project
{
	class Db;
}
namespace CheckOut
{
	class Db;
}

class FileProps
{
public:
	FileProps (FileData const & fileData, Project::Db const & projectDb);

	void AddCheckoutNotifications (Project::Db const & projectDb,
								   CheckOut::Db const & checkoutNotificationDb);

	void SetCheckoutNotifications (bool flag) { _isCheckoutNotificationOn = flag; }

	std::string GetCaption () const;
	bool IsCheckoutNotificationOn () const { return _isCheckoutNotificationOn; }

public:
	class CheckoutSequencer
	{
	public:
		CheckoutSequencer (FileProps const & props)
			: _cur (props._checkedOutBy.begin ()),
			  _end (props._checkedOutBy.end ())
		{}

		bool AtEnd () const { return _cur == _end; }
		void Advance () { ++_cur; }

		std::string const & GetMember () const { return *_cur; }

	private:
		std::vector<std::string>::const_iterator	_cur;
		std::vector<std::string>::const_iterator	_end;
	};

	friend class CheckoutSequencer;

private:
	void AddCheckedOutBy (Project::Db const & projectDb, UserId userId);

private:
	GlobalId					_gid;
	std::string					_fileName;
	bool						_isCheckoutNotificationOn;
	std::vector<std::string>	_checkedOutBy;
};


#endif
