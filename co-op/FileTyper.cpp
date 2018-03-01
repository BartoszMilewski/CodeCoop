//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include "precompiled.h"
#include "FileTyper.h"
#include "Database.h"
#include "Catalog.h"
#include "PathFind.h"
#include "resource.h"
#include "Prompter.h"
#include "UniqueName.h"

#include <Ctrl/Output.h>
#include <StringOp.h>

AssocEntry DefaultAssoc [] =
{
	{ ".h", FileType::typeHeader, true },
	{ ".hpp", FileType::typeHeader, true },
	{ ".hxx", FileType::typeHeader, true },

	{ ".c", FileType::typeSource, true },
	{ ".cpp", FileType::typeSource, true },
	{ ".cxx", FileType::typeSource, true },

	{ ".java", FileType::typeSource, true },

	{ ".inc", FileType::typeHeader, true },
	{ ".pas", FileType::typeSource, true },

	{ ".txt", FileType::typeText, true },
	{ ".wiki", FileType::typeText, true },
	{ ".htm", FileType::typeText, true },
	{ ".html", FileType::typeText, true },
	{ ".xml", FileType::typeText, true },

	{ ".gif", FileType::typeBinary, true },
	{ ".jpg", FileType::typeBinary, true },
	{ ".png", FileType::typeBinary, true },
	{ ".bmp", FileType::typeBinary, true }
};

char * AssociationList::_fileTypeNames [] =
{
	"Header File",
	"Source File",
	"Text File",
	"Binary File",
	"Auto File",
	"Wiki File"
};

AssociationList::AssociationList (Catalog & cat)
	: _catalog (cat)
{
	for (FileTypeSeq seq (cat); !seq.AtEnd (); seq.Advance ())
	{
		std::unique_ptr<Association> ass (new Association (seq.GetExt ().c_str (), seq.GetType ()));
		_list.push_back (std::move(ass));
	}
}

FileType AssociationList::GetFileType (std::string const & ext) const
{
	if (!ext.empty ())
	{
		for (unsigned int i = 0; i < _list.size (); i++)
		{
			Association const * info = _list [i];
			if (IsFileNameEqual (info->GetExtension (), ext))
				return info->GetType ();
		}
	}
	FileType invalid;
	return invalid;
}

void AssociationList::Add (std::string const & ext, FileType type)
{
	if (!ext.empty ())
	{
		_catalog.AddFileType (ext, type);
		std::unique_ptr<Association> ass (new Association (ext, type));
		_list.push_back (std::move(ass));
	}
}

char const * AssociationList::Type2Str (FileType type)
{
	if (type.IsHeader ())
		return _fileTypeNames [0];
	else if (type.IsSource ())
		return _fileTypeNames [1];
	else if (type.IsText ())
		return _fileTypeNames [2];
	else if (type.IsBinary ())
		return _fileTypeNames [3];
	else if (type.IsAuto ())
		return _fileTypeNames [4];
	else
		return "";
}

FileType AssociationList::Str2Type (char const * typeName)
{
	if (IsNocaseEqual (typeName, _fileTypeNames [0]))
		return HeaderFile ();
	else if (IsNocaseEqual (typeName, _fileTypeNames [1]))
		return SourceFile ();
	else if (IsNocaseEqual (typeName, _fileTypeNames [2]))
		return TextFile ();
	else if (IsNocaseEqual (typeName, _fileTypeNames [3]))
		return BinaryFile ();
	else if (IsNocaseEqual (typeName, _fileTypeNames [4]))
		return AutoFile ();
	else if (IsNocaseEqual (typeName, _fileTypeNames [5]))
		return TextFile ();
	FileType invalid;
	return invalid;
}

void FileTyperData::SetFileName (std::string const & fileName)
{
	_fileName = fileName;
	PathSplitter splitter (fileName);
	_ext = splitter.GetExtension ();
	_add = !_ext.empty ();
}

// command line:
// -selection_add "c:\project\file.ext" type:"text file"

bool FileTyperCtrl::GetDataFrom (NamedValues const & source)
{
	_dlgData->SetAdd (false); // don't add association
	std::string typeName = source.GetValue ("type");
	if (typeName.empty ())
		return false;
	FileType type = AssociationList::Str2Type (typeName.c_str ());
	if (type.IsInvalid ())
		return false;
	_dlgData->SetType (type);
	return true;
}

bool FileTyperCtrl::OnInitDialog () throw (Win::Exception)
{
	_extFrame.Init (GetWindow (), IDC_FILE_TYPER_EXT_LABEL);
	_ext.Init (GetWindow (), IDC_FILE_TYPER_EXT);
	_header.Init (GetWindow (), IDC_FILE_TYPER_HEADER);
	_source.Init (GetWindow (), IDC_FILE_TYPER_SOURCE);
	_text.Init (GetWindow (), IDC_FILE_TYPER_TEXT);
	_binary.Init (GetWindow (), IDC_FILE_TYPER_BINARY);
	_add.Init (GetWindow (), IDC_FILE_TYPER_ADD);

	FileType type = _dlgData->GetType ();
	if (type.IsHeader ())
		_header.Check ();
	else if (type.IsSource ())
		_source.Check ();
	else if (type.IsText ())
		_text.Check ();
	else if (type.IsBinary ())
		_binary.Check ();

	if (_dlgData->HasExtension ())
	{
		_add.Check ();
		_ext.SetString (_dlgData->GetExtension ());
		_extFrame.SetText ("File name extension: ");
	}
	else
	{
		_add.Disable ();
		_ext.SetString (_dlgData->GetFileName ());
		_extFrame.SetText ("File name without extension: ");
	}
	return true;
}

bool FileTyperCtrl::OnApply () throw ()
{
	if (_header.IsChecked ())
	{
		HeaderFile type;
		_dlgData->SetType (type);
	}
	else if (_source.IsChecked ())
	{
		SourceFile type;
		_dlgData->SetType (type);
	}
	else if (_text.IsChecked ())
	{
		TextFile type;
		_dlgData->SetType (type);
	}
	else if (_binary.IsChecked ())
	{
		BinaryFile type;
		_dlgData->SetType (type);
	}
	_dlgData->SetAdd (_add.IsChecked ());
	EndOk ();
	return true;
}

FileType FileTyper::GetFileType (std::string const & name)
{
	PathSplitter splitter (name);
	char const * ext = splitter.GetExtension ();
	FileType fileType = _associationDB.GetFileType (ext);
	if (fileType.IsInvalid ())
	{
		// Ask user
		_dlgData.SetFileName (name);
		FileTyperCtrl ctrl (&_dlgData);
		if (ThePrompter.GetData (ctrl, _inputSource))
		{
			fileType = _dlgData.GetType ();
			if (_dlgData.AddAssociation ())
				_associationDB.Add (ext, fileType);
		}
	}
	return fileType;
}

//
// Selection change file type dialog
//

FileTypeChangeData::FileTypeChangeData (std::set<std::string> const & extensions,
										GidList const & gids,
										FileIndex const & fileIndex)
	: _add (true),
	  _isTypeSelected (false)
{
	std::set<std::string>::const_iterator it = extensions.begin ();
	Assert (it != extensions.end ());
	for ( ; it != extensions.end (); ++it)
	{
		std::string const & ext = *it;
		if (ext.empty ())
		{
			_allExtensions += "'no extension'";
			_add = false;	// Don't add no extension to the association database
		}
		else
		{
			_allExtensions += ext;
		}

		_allExtensions += ' ';
	}

	GidList::const_iterator gidIter = gids.begin ();
	Assert (gidIter != gids.end ());
	FileData const * fileData = fileIndex.GetFileDataByGid (*gidIter);
	FileType firstType = fileData->GetType ();
	bool sameType = true;
	++gidIter;
	while (gidIter != gids.end ())
	{
		FileData const * fileData = fileIndex.GetFileDataByGid (*gidIter);
		sameType = firstType.IsEqual (fileData->GetType ());
		++gidIter;
	}
	if (sameType)
		_type = firstType;
}

void FileTypeChangeData::SetType (FileType newType)
{
	if (!_type.IsEqual (newType))
	{
		_type = newType;
		_isTypeSelected = true;
	}
}

bool FileTypeChangerCtrl::OnInitDialog () throw (Win::Exception)
{
	_ext.Init (GetWindow (), IDC_FILE_TYPE_EXT);
	_header.Init (GetWindow (), IDC_FILE_TYPE_HEADER);
	_source.Init (GetWindow (), IDC_FILE_TYPE_SOURCE);
	_text.Init (GetWindow (), IDC_FILE_TYPE_TEXT);
	_binary.Init (GetWindow (), IDC_FILE_TYPE_BINARY);
	_add.Init (GetWindow (), IDC_FILE_TYPE_ADD);

	_ext.SetString (_dlgData->GetExtensions ());
	FileType type = _dlgData->GetType ();
	if (type.IsHeader ())
		_header.Check ();
	else if (type.IsSource ())
		_source.Check ();
	else if (type.IsText ())
		_text.Check ();
	else if (type.IsBinary ())
		_binary.Check ();
	if (_dlgData->AddAssociation ())
		_add.Check ();
	else
		_add.Disable ();
	return true;
}

bool FileTypeChangerCtrl::OnApply () throw ()
{
	if (_header.IsChecked ())
	{
		HeaderFile type;
		_dlgData->SetType (type);
	}
	else if (_source.IsChecked ())
	{
		SourceFile type;
		_dlgData->SetType (type);
	}
	else if (_text.IsChecked ())
	{
		TextFile type;
		_dlgData->SetType (type);
	}
	else if (_binary.IsChecked ())
	{
		BinaryFile type;
		_dlgData->SetType (type);
	}
	_dlgData->SetAdd (_add.IsChecked ());
	EndOk ();
	return true;
}

// command line:
// -selection_ChangeFileType "c:\project\file.ext" type:"text file"

bool FileTypeChangerCtrl::GetDataFrom (NamedValues const & source)
{
	_dlgData->SetAdd (false); // don't add association
	std::string typeName = source.GetValue ("type");
	if (typeName.empty ())
		return false;
	FileType type = AssociationList::Str2Type (typeName.c_str ());
	if (type.IsInvalid ())
		return false;
	_dlgData->SetType (type);
	return true;
}

