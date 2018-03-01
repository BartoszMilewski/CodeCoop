#if !defined (DISTRIBUTORSCRIPTSEND_H)
#define DISTRIBUTORSCRIPTSEND_H
//---------------------------
// (c) Reliable Software 2005
//---------------------------
// A dispatcher script transerring a distributor license block is created
// and compressed to a .znc file.
// If address not empty, the file is sent as an attachment
// otherwise it is deposited on the desktop.

void MailOrSaveDistributorBlock (
								 std::string const & address,
								 std::string const & senderUserId, 
								 std::string const & senderHubId,
								 std::string const & licensee,
								 unsigned startNumber,
								 unsigned count,
								 std::string const & instructions,
								 bool isPreviewBeforeSending);

#endif
