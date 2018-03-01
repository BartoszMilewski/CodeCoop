#if !defined (CHECKOUTNOTIFICATIONS_H)
#define CHECKOUTNOTIFICATIONS_H
//-------------------------------------
//  (c) Reliable Software, 2007 -- 2009
//-------------------------------------

#include "Serialize.h"
#include "ScriptSerialize.h"
#include "Transact.h"
#include "GlobalId.h"
#include "SerVector.h"
#include "XArray.h"
#include "Params.h"

namespace CheckOut
{
	class List : public ScriptSerializable
	{
	public:
		List (Deserializer & in)
			: _senderId (gidInvalid)
		{
			Read (in);
		}
		List (GidList const & fileIds);

		void SetSenderId (UserId id) { _senderId = id; }
		UserId GetSenderId () const { return _senderId; }
		GidList const & GetFileList () const { return _fileIds; }

		bool IsSection () const { return true; }
		int  SectionId () const { return 'CHDR'; }
		int  VersionNo () const { return scriptVersion; }
		void Serialize (Serializer& out) const;
		void Deserialize (Deserializer& in, int version);

	private:
		SerVector<GlobalId>	_fileIds;
		UserId				_senderId;
	};

	class FileUsers : public Serializable
	{
	public:
		FileUsers (GlobalId fileId, GlobalId userId)
			: _fileId (fileId)
		{
			_userIds.push_back (userId);
		}
		FileUsers (Deserializer & in, int version)
		{
			Deserialize (in, version);
		}
		GlobalId GetFileId () const { return _fileId; }
		std::vector<UserId> const & GetUserIds () const { return _userIds; }

		void AddUserId (GlobalId userId);
		void RemoveUserId (GlobalId userId);
		bool IsEmpty () const { return _userIds.empty (); }

		void Serialize (Serializer & out) const;
		void Deserialize (Deserializer & in, int version);

	private:
		typedef SerVector<UserId>::iterator Iterator;

		GlobalId			_fileId;
		SerVector<UserId>	_userIds;
	};

	class Db : public TransactableContainer
	{
	public:
		Db ()
		{
			AddTransactableMember (_notes);
		}

		class Sequencer
		{
		public:
			Sequencer (CheckOut::Db const & db, GlobalId gid);

			bool AtEnd () const { return _atEnd; }
			void Advance ()
			{
				++_cur;
				_atEnd = (_cur == _end);
			}

			UserId GetUserId () const { return *_cur; }

		private:
			std::vector<UserId>::const_iterator	_cur;
			std::vector<UserId>::const_iterator	_end;
			bool								_atEnd;
		};

		friend class Sequencer;

		bool IsCheckedOut (GlobalId gid) const;
		bool IsCheckedOutBy (UserId userId) const;
		void GetNotifyingMembers (GidList & members) const;

		void XUpdate (GidList const & checkOutFiles, UserId senderId);
		void XPrepareForBranch ();

		// Transactable interface
		void		Clear () throw ();

		void        Serialize (Serializer& out) const;
		void        Deserialize (Deserializer& in, int version);
		bool        IsSection () const { return true; }
		int         SectionId () const { return 'COUT'; }
		int         VersionNo () const { return modelVersion; }

	private:
		void RefreshIndex () const;
		void XUpdateCheckoutNotes (GidList & incomingCheckoutFiles, UserId senderId);

		typedef TransactableArray<FileUsers>::const_iterator iterator;
	private:
		TransactableArray<FileUsers>			_notes;
		mutable std::map<GlobalId, unsigned>	_noteIndex;
	};
}

#endif
