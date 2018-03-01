//-----------------------------------------------------
//  (c) Reliable Software 2001 -- 2004
//-----------------------------------------------------

#include "precompiled.h"
#undef DBG_LOGGING
#define DBG_LOGGING false

#include "MapiMessage.h"
#include "MapiStore.h"
#include "MapiBuffer.h"
#include "MapiProperty.h"
#include "MapiAddrList.h"
#include "MapiEx.h"
#include "MapiGPF.h"

#include <File/Path.h>
#include <File/SafePaths.h>
#include <Dbg/Assert.h>
#include <Dbg/Out.h>

#include <mapiutil.h>
#include <mapival.h>

namespace Mapi
{
	Message::Message (Folder & folder)
	{
		folder.CreateMessage (_msg);
		// All created messges are of InterPersonal Message class
		MessageClass prop ("IPM.Note");
		SetProperty (prop);
		dbg << "    Open message. refcount = " << _msg.GetRefCount () << std::endl;
	}

	Message::Message (Folder & folder, std::vector<unsigned char> const & id)
	{
		folder.OpenMessage (id, _msg);
	}

	Message::~Message ()
	{
		dbg << "        Close message. refcount = " << _msg.GetRefCount () << std::endl;
	}

	void Message::GetAttachmentTable (Interface<IMAPITable> & table)
	{
		Result result;
		try
		{
			result = _msg->GetAttachmentTable (0,		// [in] Bitmask of flags that relate to the creation of the table. 
											&table);	// [out] Pointer to a pointer to the attachment table. 
		}
		catch (...)
		{
			Mapi::HandleGPF ("IMessage::GetAttachmentTable");
			throw;
		}
		_msg.ThrowIfError (result, "MAPI -- Cannot open message attachment table.");
	}

	void Message::OpenAttachment (unsigned long attNum, Interface<IAttach> & attach)
	{
		Result result;
		try
		{
			result = _msg->OpenAttach (attNum,	// [in] Index number of the attachment to open.
										  0,		// [in] Pointer to the interface identifier (IID)
													// representing the interface to be used to access the attachment.
													// Passing NULL results in the attachment's standard interface,
													// or IAttach, being returned.
										  0,		// [in] Bitmask of flags that controls how the attachment is opened.
										  &attach);	// [out] Pointer to a pointer to the open attachment.
		}
		catch (...)
		{
			Mapi::HandleGPF ("IMessage::OpenAttach");
			throw;
		}
		_msg.ThrowIfError (result, "MAPI -- Cannot open message attachment.");
	}

	void Message::CreateAttachment (unsigned long & attNum, Interface<IAttach> & attach)
	{
		Result result;
		try
		{
			result = _msg->CreateAttach (0,	// [in] Pointer to the interface identifier (IID) representing the interface
												// to be used to access the message. Passing NULL results in the message's standard
												// interface, or IMessage, being returned.
											0,	// [in] Bitmask of flags that controls how the attachment is created.
											&attNum,
												// [out] Pointer to an index number identifying the newly created attachment.
												// This number is valid only when the message is open and is the basis for
												// the attachment's PR_ATTACH_NUM property.
											&attach);
												// [out] Pointer to a pointer to the open attachment object.
		}
		catch (...)
		{
			Mapi::HandleGPF ("IMessage::CreateAttach");
			throw;
		}
		_msg.ThrowIfError (result, "MAPI -- Cannot create message attachment.");
	}

	void Message::ReadEnvelope (std::string & subject, std::string & sender, std::string & email)
	{
		RetrievedProperty values;
		PropertyList props;
		props.Add (PR_SUBJECT);
		props.Add (PR_SENDER_NAME);
		props.Add (PR_SENDER_EMAIL_ADDRESS);
		Result result;
		try
		{
			result = _msg->GetProps (props.Cnv2TagArray (),	// [in] Pointer to an array of property tags
														// identifying the properties whose values are to be retrieved.
								0,						// [in] Bitmask of flags that indicates the format for properties
														// that have the type PT_UNSPECIFIED.
								values.GetCountBuf (),	// [out] Pointer to a count of property values pointed
														// to by the lppPropArray parameter.
								values.GetBuf ());		// [out] Pointer to a pointer to the retrieved property values.
		}
		catch (...)
		{
			Mapi::HandleGPF ("IMessage::GetProps");
			throw;
		}
		_msg.ThrowIfError (result, "MAPI -- Cannot retrieve message envelope.");
		Assert (values.GetCount () == 3);
		if (values [0].Value.lpszA != reinterpret_cast<char *>(MAPI_E_NOT_FOUND))
			subject.assign (values [0].Value.lpszA);
		if (values [1].Value.lpszA != reinterpret_cast<char *>(MAPI_E_NOT_FOUND))
			sender.assign (values [1].Value.lpszA);
		if (values [2].Value.lpszA != reinterpret_cast<char *>(MAPI_E_NOT_FOUND))
			email.assign (values [2].Value.lpszA);
	}

	void Message::GetId (Buffer<SPropValue> & buf)
	{
		Result result;
		try
		{
			result = ::HrGetOneProp (_msg,	// [in] Pointer to the IMAPIProp interface from which the
												// property value is to be retrieved.
								PR_ENTRYID,	// [in] Property tag of the property to be retrieved.
								&buf);			// [out] Pointer to a pointer to the returned SPropValue structure
												// defining the retrieved property value.
		}
		catch (...)
		{
			Mapi::HandleGPF ("::HrGetOneProp (IMessage)");
			throw;
		}
		_msg.ThrowIfError (result, "MAPI -- Cannot access message entry id.");
	}

	void Message::AddRecipients (AddrList const & addrList)
	{
		Result result;
		try
		{
			result = _msg->ModifyRecipients (MODRECIP_ADD, addrList.GetBuf ());
		}
		catch (...)
		{
			Mapi::HandleGPF ("IMessage::ModifyRecipients");
			throw;
		}
		_msg.ThrowIfError (result, "MAPI -- Cannot set message recipients.");
	}

	void Message::AddAttachment (std::string const & path)
	{
		// Create new attachment for this message
		Attachment att (*this);
		// Store file in the attachment
		att.Store (path.c_str ());
		att.SaveChanges ();
		// Save changes made to the message
		SaveChanges ();
	}

	void Message::SetSubject (std::string const & subject)
	{
		Subject prop (subject);
		SetProperty (prop);
	}

	void Message::SetNoteText (std::string const & noteText)
	{
		NoteText prop (noteText);
		SetProperty (prop);
	}

	void Message::SetClass (std::string const & cls)
	{
		MessageClass prop (cls);
		SetProperty (prop);
	}

	void Message::MarkHasAttachment ()
	{
		MsgHasAttachment hasAttach (true);
		SetProperty (hasAttach);
	}

	void Message::MarkAsRead ()
	{
		Result result;
		try
		{
			result = _msg->SetReadFlag (0);	// Sets message read flag in PR_MESSAGE_FLAGS property
		}
		catch (...)
		{
			Mapi::HandleGPF ("IMessage::SetReadFlag");
			throw;
		}
		_msg.ThrowIfError (result, "MAPI -- Cannot mark message as read.");
	}

	void Message::SaveChanges ()
	{
		Result result;
		try
		{
			result = _msg->SaveChanges (KEEP_OPEN_READWRITE);
			if (result.ChangesDetected ())
			{
				// In the mean time message have been changed by some other MAPI client.
				// Force our changes.
				result = _msg->SaveChanges (KEEP_OPEN_READWRITE | FORCE_SAVE);
			}
		}
		catch (...)
		{
			Mapi::HandleGPF ("IMessage::SaveChanges");
			throw;
		}
		_msg.ThrowIfError (result, "MAPI -- Cannot save message changes.");
	}

	void Message::SetSentOptions (FolderEntryId const & sentItemsId)
	{
		if (sentItemsId.IsValid ())
		{
			SentItemsId prop (sentItemsId.GetSize (), reinterpret_cast<unsigned char const *>(sentItemsId.Get ()));
			SetProperty (prop);
			Assert (::FPropExists (_msg, PR_SENTMAIL_ENTRYID));
		}
	}

	void Message::Submit ()
	{
		Result result;
		try
		{
			result = _msg->SubmitMessage (0);
		}
		catch (...)
		{
			Mapi::HandleGPF ("IMessage::SubmitMessage");
			throw;
		}
		_msg.ThrowIfError (result, "MAPI -- Cannot submit message.");
	}

	void Message::SetProperty (SPropValue const * prop)
	{
		Result result;
		try
		{
			result = _msg->SetProps (1,// [in] Count of property values pointed to by the next parameter.
							const_cast<SPropValue *>(prop),
										// [in] Pointer to an array of SPropValue structures holding
										// property values to be updated.
							0);		// [in, out] On input, can be NULL, indicating no need for error
										// information, or a pointer to a pointer to an SPropProblemArray
										// structure. If this is a valid pointer on input, SetProps returns
										// detailed information about errors in updating one or more properties.
		}
		catch (...)
		{
			Mapi::HandleGPF ("IMessage::SetProps");
			throw;
		}
		_msg.ThrowIfError (result, "MAPI -- Cannot set message property.");
	}

	Envelope::Envelope (Message & msg)
	{
		msg.ReadEnvelope (_subject, _senderName, _senderEmail);
	}

	AttachmentTable::AttachmentTable (Message & msg)
		: _msg (msg)
	{
		_msg.GetAttachmentTable (_table);
	}

	void AttachmentTable::SaveAttachments (SafePaths & attPaths)
	{
		// We are interested only in the attachment number property
		PropertyList columns;
		columns.Add (PR_ATTACH_NUM);
		RowSet rows;
		Result result;
		try
		{
			result = ::HrQueryAllRows (_table,	// [in] Pointer to the MAPI table from which rows are retrieved.
								columns.Cnv2TagArray (),
													// [in] Pointer to an SPropTagArray structure containing an array
													// of property tags indicating table columns. These tags are used
													// to select the specific columns to be retrieved. If the ptaga
													// parameter is NULL, HrQueryAllRows retrieves the entire column
													// set of the current table view passed in the ptable parameter.
								0,				// [in] Pointer to an SRestriction structure containing retrieval restrictions.
													// If the pres parameter is NULL, HrQueryAllRows makes no restrictions.
								0,				// [in] Pointer to an SSortOrderSet structure identifying the sort order of the columns to be retrieved.
													// If the psos parameter is NULL, the default sort order for the table is used.
								0,				// [in] Maximum number of rows to be retrieved. If the value of the
													// crowsMax parameter is zero, no limit on the number of rows retrieved is set.
								&rows);			// [out] Pointer to a pointer to the returned SRowSet structure containing
													// an array of pointers to the retrieved table rows.
		}
		catch (...)
		{
			Mapi::HandleGPF ("::HrQueryAllRows (IMAPITable)");
			throw;
		}
		_table.ThrowIfError (result, "MAPI -- Cannot retrieve message attachment index.");
		Assert (!FBadRowSet (rows.Get ()));
		TmpPath tmpPath;
		for (unsigned int i = 0; i < rows.Count (); ++i)
		{
			unsigned long attachNum = rows.GetAttachNum (i);
			Attachment attach (_msg, attachNum);
			if (attach.IsFile ())
			{
				std::string fileName = attach.GetFileName ();
				char const * path = tmpPath.GetFilePath (fileName);
				attach.Save (path);
				attPaths.Remember (path);
			}
		}
	}

	Attachment::Attachment (Message & msg, unsigned long attachNum)
		: _attNum (attachNum)
	{
		msg.OpenAttachment (_attNum, _attach);
	}

	Attachment::Attachment (Message & msg)
	{
		msg.CreateAttachment (_attNum, _attach);
		// Set attachment number
		AttachmentNumber prop (_attNum);
		SetProperty (prop);
		// Set attachment method
		AttachByValue method;
		SetProperty (method);
		// Set rendering position to irrelevant
		RenderingPosition pos (-1);
		SetProperty (pos);
	}

	void Attachment::SetProperty (SPropValue const * prop)
	{
		Result result;
		try
		{
			result = _attach->SetProps (1,// [in] Count of property values pointed to by the next parameter.
							const_cast<SPropValue *>(prop),
										// [in] Pointer to an array of SPropValue structures holding
										// property values to be updated.
							0);		// [in, out] On input, can be NULL, indicating no need for error
										// information, or a pointer to a pointer to an SPropProblemArray
										// structure. If this is a valid pointer on input, SetProps returns
										// detailed information about errors in updating one or more properties.
		}
		catch (...)
		{
			Mapi::HandleGPF ("IAttach::SetProps");
			throw;
		}
		_attach.ThrowIfError (result, "MAPI -- Cannot set attachment property.");
	}

	void Attachment::CreateBuffer (Com::UnknownPtr & stream)
	{
		// Create the property of the attachment containing the data
		// Request IStream interface to attachment data
		Result result;
		try
		{
			result = _attach->OpenProperty (PR_ATTACH_DATA_BIN,
														// [in] Property tag for the property to be accessed.
									&IID_IStream,	// [in] Pointer to the identifier for the interface to be used to access the property.
									0,				// [in] Data that relates to the interface identified by the lpiid parameter.
									MAPI_CREATE | MAPI_MODIFY,
														// [in] Bitmask of flags that controls access to the property.
									&stream);		// [out] Pointer to the requested interface to be used for property access.
		}
		catch (...)
		{
			Mapi::HandleGPF ("IAttach::OpenProperty");
			throw;
		}
		_attach.ThrowIfError (result, "MAPI -- Cannot create attachment data.");
	}

	void Attachment::OpenBuffer (Com::UnknownPtr & stream)
	{
		// Open the property of the attachment containing the data
		// Request IStream interface to attachment data
		Result result;
		try
		{
			result = _attach->OpenProperty (PR_ATTACH_DATA_BIN,
														// [in] Property tag for the property to be accessed.
									&IID_IStream,	// [in] Pointer to the identifier for the interface to be used to access the property.
									0,				// [in] Data that relates to the interface identified by the lpiid parameter.
									0,				// [in] Bitmask of flags that controls access to the property.
									&stream);		// [out] Pointer to the requested interface to be used for property access.
		}
		catch (...)
		{
			Mapi::HandleGPF ("IAttach::OpenProperty");
			throw;
		}
		_attach.ThrowIfError (result, "MAPI -- Cannot open attachment data.");
	}

	void Attachment::SaveChanges ()
	{
		Result result;
		try
		{
			result = _attach->SaveChanges (KEEP_OPEN_READWRITE);
			if (result.ChangesDetected ())
			{
				// In the mean time attachment have been changed by some other MAPI client.
				// Force our changes.
				result = _attach->SaveChanges (KEEP_OPEN_READWRITE | FORCE_SAVE);
			}
		}
		catch (...)
		{
			Mapi::HandleGPF ("IAttach::SaveChanges");
			throw;
		}
		_attach.ThrowIfError (result, "MAPI -- Cannot save attachment changes.");
	}

	bool Attachment::IsFile ()
	{
		RetrievedProperty value;
		PropertyList props;
		props.Add (PR_ATTACH_METHOD);
		Result result;
		try
		{
			result = _attach->GetProps (props.Cnv2TagArray (),	// [in] Pointer to an array of property tags
														// identifying the properties whose values are to be retrieved.
							0,						// [in] Bitmask of flags that indicates the format for properties
														// that have the type PT_UNSPECIFIED.
							value.GetCountBuf (),	// [out] Pointer to a count of property values pointed
														// to by the lppPropArray parameter.
							value.GetBuf ());		// [out] Pointer to a pointer to the retrieved property values.
		}
		catch (...)
		{
			Mapi::HandleGPF ("IAttach::GetProps");
			throw;
		}
		_attach.ThrowIfError (result, "MAPI -- Cannot retrieve attachment access method.");
		Assert (value.GetCount () == 1);
		bool isFileAttach = false;
		if (value [0].Value.ul != MAPI_E_NOT_FOUND)
			isFileAttach = value [0].Value.ul == ATTACH_BY_VALUE;
		return isFileAttach;
	}

	std::string Attachment::GetFileName ()
	{
		RetrievedProperty values;
		PropertyList props;
		props.Add (PR_ATTACH_FILENAME);
		props.Add (PR_ATTACH_LONG_FILENAME);
		Result result;
		try
		{
			result = _attach->GetProps (props.Cnv2TagArray (),	// [in] Pointer to an array of property tags
														// identifying the properties whose values are to be retrieved.
							0,						// [in] Bitmask of flags that indicates the format for properties
														// that have the type PT_UNSPECIFIED.
							values.GetCountBuf (),	// [out] Pointer to a count of property values pointed
														// to by the lppPropArray parameter.
							values.GetBuf ());		// [out] Pointer to a pointer to the retrieved property values.
		}
		catch (...)
		{
			Mapi::HandleGPF ("IAttach::GetProps");
			throw;
		}
		_attach.ThrowIfError (result, "MAPI -- Cannot retrieve attachment file name.");
		Assert (values.GetCount () == 2);
		std::string fileName;
		std::string longFileName;
		if (values [0].Value.lpszA != reinterpret_cast<char *>(MAPI_E_NOT_FOUND))
			fileName.assign (values [0].Value.lpszA);
		if (values [1].Value.lpszA != reinterpret_cast<char *>(MAPI_E_NOT_FOUND))
			longFileName.assign (values [1].Value.lpszA);
		if (longFileName.empty ())
			return fileName;
		else
			return longFileName;
	}

	void Attachment::Save (char const * path)
	{
		Buffer buf;
		buf.Open (*this);
		File target;
		target.Create (path);
		target.Write (buf, buf.size ());
	}

	void Attachment::Store (std::string const & path)
	{
		// Set file name properties
		{
			// Short file name
			PathSplitter splitter (path);
			std::string name (splitter.GetFileName ());
			name += splitter.GetExtension ();
			FileName fn (name);
			SetProperty (fn);
			FileNameExtension ext (splitter.GetExtension ());
			SetProperty (ext);
		}
		{
			// Long file name
			FilePath tmpPath (path);
			tmpPath.ConvertToLongPath ();
			PathSplitter splitter (tmpPath.GetDir ());
			std::string name (splitter.GetFileName ());
			name += splitter.GetExtension ();
			LongFileName fn (name);
			SetProperty (fn);
		}
		// Set contents properties
		AttachTagMime tag;
		SetProperty (tag);
		MimeTag mimeTag ("application/octet-stream");
		SetProperty (mimeTag);
		Buffer buf;
		buf.Create (*this);
		File source;
		source.Open (path.c_str ());
		source.Read (buf);
	}

	void Attachment::Buffer::Open (Attachment & attach)
	{
		attach.OpenBuffer (*this);
	}

	void Attachment::Buffer::Create (Attachment & attach)
	{
		attach.CreateBuffer (*this);
	}

	unsigned long Attachment::Buffer::size ()
	{
		// Get attachment data size -- don't request name
		STATSTG stats;
		(*this)->Stat (&stats, STATFLAG_NONAME);

		Assert (stats.cbSize.HighPart == 0);
		return stats.cbSize.LowPart;
	}

	void Attachment::File::Open (char const * path)
	{
		HRESULT hRes = ::OpenStreamOnFile (::MAPIAllocateBuffer,
										::MAPIFreeBuffer,
										STGM_READ,
										const_cast<char *>(path),
										0,
										&(*this));
		if (FAILED (hRes))
			throw Exception ("MAPI -- Cannot open attachment file.", hRes);
	}

	void Attachment::File::Create (char const * path)
	{
		HRESULT hRes = ::OpenStreamOnFile (::MAPIAllocateBuffer,
										::MAPIFreeBuffer,
										STGM_CREATE | STGM_READWRITE,
										const_cast<char *>(path),
										0,
										&(*this));
		if (FAILED (hRes))
			throw Exception ("MAPI -- Cannot create attachment file.", hRes);
	}

	void Attachment::File::Write (Com::IfacePtr<IStream> & buf, unsigned long size)
	{
		ULARGE_INTEGER bufSize;
		bufSize.HighPart = 0;
		bufSize.LowPart = size;
		ULARGE_INTEGER readFromSource;
		ULARGE_INTEGER writtenToDestination;
		// Copy buffer to the file
		HRESULT hRes = buf->CopyTo (*this, bufSize, &readFromSource, &writtenToDestination);
		if (FAILED (hRes))
			throw Exception ("MAPI -- Cannot save attachment.", hRes);
		Assert (readFromSource.QuadPart == bufSize.QuadPart);
		Assert (readFromSource.QuadPart == writtenToDestination.QuadPart);
		// Commit changes to the destination file
		(*this)->Commit (0);
	}

	void Attachment::File::Read (Com::IfacePtr<IStream> & buf)
	{
		// Get attachment file size -- don't request name
		STATSTG stats;
		(*this)->Stat (&stats, STATFLAG_NONAME);

		ULARGE_INTEGER readFromSource;
		ULARGE_INTEGER writtenToDestination;
		// Copy file to the buffer
		HRESULT hRes = (*this)->CopyTo (buf, stats.cbSize, &readFromSource, &writtenToDestination);
		if (FAILED (hRes))
			throw Exception ("MAPI -- Cannot store attachment binary data.", hRes);
		Assert (readFromSource.QuadPart == stats.cbSize.QuadPart);
		Assert (readFromSource.QuadPart == writtenToDestination.QuadPart);
	}
}