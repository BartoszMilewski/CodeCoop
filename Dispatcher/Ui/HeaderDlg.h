#if !defined (HEADERDLG_H)
#define HEADERDLG_H
// ----------------------------------
// (c) Reliable Software, 1999 - 2005
// ----------------------------------

#include "Addressee.h"
#include "TransportData.h"
#include "UserIdPack.h"

#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/ListView.h>
#include <Ctrl/Button.h>

class HeaderCtrl : public Dialog::ControlHandler
{
public:
    HeaderCtrl (TransportData const & data, std::string const & filename);

    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
    class AddRow
    {
    public:
        AddRow (Win::ReportListing & listing) 
            : _listing (listing)
        {}
        void operator () (Addressee const & addressee) 
        {
			int item = _listing.AppendItem (addressee.GetHubId ().c_str ());
			UserIdPack pack (addressee.GetStringUserId ());
            _listing.AddSubItem (pack.GetUserIdString ().c_str (), item, 1);
            _listing.AddSubItem (addressee.ReceivedScript () ? "Delivered" : "Not delivered", item, 2);
        }
    private:
		Win::ReportListing & _listing;
    };

private:
	TransportData const & _data;
	std::string const & _scriptFilename;

    Win::Edit			_filename;
    Win::Edit			_project;
    Win::Edit			_hubId;
    Win::Edit			_userId;
    Win::Edit			_status;
	Win::ReportListing	_recipients;
    Win::Edit			_comment;
    Win::CheckBox		_forward;
    Win::CheckBox		_defect;
};

#endif
