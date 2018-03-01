#if !defined (PROJECTSEATS_H)
#define PROJECTSEATS_H
//---------------------------------------------
// (c) Reliable Software 2005
//---------------------------------------------

#include <auto_vector.h>

class MemberInfo;

namespace Project
{
	class Db;

	class Seats
	{
	public:
		Seats ()
			: _missing (0)
		{}
		Seats (Project::Db const & projectDb, std::string const & additionalLicense);
		Seats (std::vector<MemberInfo> const & memberData, std::string const & additionalLicense);

		void Refresh (Project::Db const & projectDb);
		void XRefresh (Project::Db const & projectDb);
		void Clear () { _missing = 0; }

		bool IsEnoughLicenses () const { return _missing == 0; }
		int GetMissing () const { return _missing; }

	private:
		class NameCount
		{
		public:
			NameCount (std::string const & name, int seats, int version, char prodId)
				: _name (name),
				_seats (seats),
				_version(version),
				_productId(prodId),
				_members (1)
			{}
			bool IsEqual (std::string const & name, int version, char prodId)
			{
				return name == _name && _version == version && (version >= 5? prodId == _productId: true);
			}
			int GetMissing () const 
			{ 
				if (_members > _seats)
					return _members - _seats;
				else
					return 0;
			}
			void Update (int seats) 
			{
				if (seats > _seats)
					_seats = seats;
				_members++;
			}
		private:
			std::string	_name;
			int			_version;
			char		_productId;
			int			_seats;
			int			_members;
		};

		class LicenseArray
		{
		public:
			void Add (std::string const & name, int seats, int version, char prodId);
			int GetMissing () const;

		private:
			std::vector<NameCount> _arr;
		};

	private:
		void CountSeats (std::vector<MemberInfo> const & memberData, std::string const & additionalLicense);

	private:
		int	_missing;
	};
}

#endif
