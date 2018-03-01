//------------------------------------
//  (c) Reliable Software, 2007 - 2008
//------------------------------------

#include "precompiled.h"
#include "ExportImportHistoryDlg.h"

#include <File/File.h>

History::ExportRequest::ExportRequest (std::string const & projectName)
	: SaveFileRequest ("Export Code Co-op Project History",
					   "Exported history file name:  ",
					   "Store the project history on:  ",
					   "Recent History Export",
					   "History Export FTP Site",
					   "History Export Path")
{
	_fileName = projectName;
	_fileName += ".his";
	File::LegalizeName (_fileName);
	_fileDescription = "project history";

	_userNote = "Note: The history you are exporting can only be imported in enlistments that have already joined this project."
				" Future joins will not be able to accept this history.";
}

History::OpenRequest::OpenRequest ()
	: OpenFileRequest ("Open Code Co-op Project History File",
					   "The project history is present on:  ",
					   "Select History File To Open",
					   "Recent History Import",
					   "History Import FTP Site",
					   "History Import Path")
{}

