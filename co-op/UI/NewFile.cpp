//------------------------------------
//  (c) Reliable Software, 1997 - 2008
//------------------------------------

#include "precompiled.h"
#include "NewFile.h"
#include "resource.h"
#include "Serialize.h"
#include "OutputSink.h"

#include <File/Path.h>
#include <Ctrl/Output.h>
#include <StringOp.h>

#include <locale>

bool NewFileData::IsNameOK ()
{
	if (!_name.empty ())
	{
		PathSplitter splitter (_name);
		if (splitter.HasOnlyFileName ())
		{
			FilePath path (_targetFolder);
			return !File::Exists (path.GetFilePath (_name));
		}
	}
	return false;
}

void NewFileData::DisplayNameErrors ()
{
	if (_name.empty ())
	{
		TheOutput.Display ("Please enter the new file name.", Out::Information);
		return;
	}
	PathSplitter splitter (_name);
	if (!splitter.HasOnlyFileName ())
	{
		TheOutput.Display ("Please specify only a file name. If you want to create a new file\n"
						   "in another folder, then close this dialog, go to the appropriate folder\n"
						   "and again select Folder>New File.");
		return;
	}
	FilePath path (_targetFolder);
	if (File::Exists (path.GetFilePath (_name)))
	{
		std::string info ("Cannot create '");
		info += _name;
		info += "'. A file with the same name already exists in the target folder\n";
		info += _targetFolder;
		info += "\n\nPlease specify a different file name.";
		TheOutput.Display (info.c_str ());
	}
}

void NewFileData::Serialize (FileSerializer & out) const
{
    if (UseDefine ())
	{
		std::string ifDefString;
		std::string nameUpperCase (_name.c_str ());
		for (size_t i = 0; i < nameUpperCase.length (); i++)
		{
			if (!::IsAlnum (nameUpperCase [i]) || ::IsSpace (nameUpperCase [i]))
				nameUpperCase [i] = '_';
			else
				nameUpperCase [i] = ::ToUpper (nameUpperCase [i]);
		}

		ifDefString += "#if !defined (";
		ifDefString += nameUpperCase.c_str ();
		ifDefString += ")\r\n";
		ifDefString += "#define ";
		ifDefString += nameUpperCase;
		ifDefString += "\r\n";
		out.PutBytes (ifDefString.c_str (), ifDefString.length ());
	}
    if (UseContent ())
        out.PutBytes (_content.c_str (), _content.length ());
    if (UseDefine ())
	{
		std::string endIf ("\r\n\r\n#endif\r\n");
		out.PutBytes (endIf.c_str (), endIf.length ());
	}
}

void NewFileData::SetCopyright (std::string const & newCopyright)
{
	if (_content != newCopyright)
	{
		_content.assign (newCopyright);
		_newCopyright = true;
	}
}

NewFileCtrl::NewFileCtrl (NewFileData * data)
	: Dialog::ControlHandler (IDD_NEW_FILE),
	  _dlgData (data),
	  _copyrightDC (_copyright),
	  _copyrightFont (9, "Courier New"),
	  _copyrightFace (_copyrightFont.Create ()),
	  _copyrightHolder (_copyrightDC, _copyrightFace)
{}

bool NewFileCtrl::OnInitDialog () throw (Win::Exception)
{
	_fname.Init (GetWindow (), IDC_NEW_FILE_FNAME_EDIT);
	_path.Init (GetWindow (), IDC_NEW_FILE_PATH);
	_header.Init (GetWindow (), IDC_NEW_FILE_HEADER);
	_source.Init (GetWindow (), IDC_NEW_FILE_SOURCE);
	_other.Init (GetWindow (), IDC_NEW_FILE_OTHER);
	_binary.Init (GetWindow (), IDC_NEW_FILE_BINARY);
	_define.Init (GetWindow (), IDC_NEW_FILE_DEFINE);
	_copyright.Init (GetWindow (), IDC_NEW_FILE_COPYRIGHT_EDIT);
	_default.Init (GetWindow (), IDC_NEW_FILE_DEFAULT);
	_useCopyright.Init (GetWindow (), IDC_NEW_FILE_USE_COPYRIGHT);

    if (_dlgData->GetType ().IsHeader ())
        _header.Check ();
	else if (_dlgData->GetType ().IsSource ())
        _source.Check ();
	else if (_dlgData->GetType ().IsText ())
        _other.Check ();
	else if (_dlgData->GetType ().IsBinary ())
        _binary.Check ();

    if (_dlgData->UseDefine ())
        _define.Check ();
    else
        _define.UnCheck ();
    if (_dlgData->IsDefault ())
        _default.Check ();
    else
        _default.UnCheck ();
    if (_dlgData->UseContent ())
        _useCopyright.Check ();
    else
        _useCopyright.UnCheck ();

    _path.SetString (_dlgData->GetTargetFolder ());

    // For copyright multi-line edit use Courier New font

    _copyright.SetFont (_copyrightFace);

    if (!_dlgData->GetContent ().empty ())
        _copyright.SetString (_dlgData->GetContent ().c_str ());
	return true;
}

bool NewFileCtrl::GetDataFrom (NamedValues const & source)
{
	std::string fullPath = source.GetValue ("FileName");
	return true;
}

bool NewFileCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_NEW_FILE_HEADER:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			HeaderFile type;
			_dlgData->SetType (type);
			_define.Enable ();
			_useCopyright.Enable ();
			_copyright.Enable ();
			_default.Enable ();
			_default.Check ();
			_define.Enable ();
			_define.Check ();
			_useCopyright.Check ();
			return true;
		}
		break;
	case IDC_NEW_FILE_SOURCE:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			SourceFile type;
			_dlgData->SetType (type);
			_define.Enable ();
			_useCopyright.Enable ();
			_copyright.Enable ();
			_default.Enable ();
			_default.Check ();
			_define.UnCheck ();
			_define.Disable ();
			_useCopyright.Check ();
			return true;
		}
		break;
	case IDC_NEW_FILE_OTHER:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			TextFile type;
			_dlgData->SetType (type);
			_define.Enable ();
			_useCopyright.Enable ();
			_copyright.Enable ();
			_default.Enable ();
			_default.UnCheck ();
			_define.UnCheck ();
			_define.Disable ();
			_useCopyright.UnCheck ();
			return true;
		}
		break;
	case IDC_NEW_FILE_BINARY:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			BinaryFile type;
			_dlgData->SetType (type);
			_define.UnCheck ();
			_define.Disable ();
			_useCopyright.Disable ();
			_copyright.Disable ();
			_default.Disable ();
			return true;
		}
		break;
	case IDC_NEW_FILE_COPYRIGHT_EDIT:
		if (_copyright.IsChanged (notifyCode))
		{
			if (_copyright.GetLen () == 0)
			{
				_useCopyright.UnCheck ();
			}
			else
			{
				_useCopyright.Check ();
			}
		}
		break;
	}
    return false;
}

bool NewFileCtrl::OnApply () throw ()
{
	_dlgData->SetName (_fname.GetTrimmedString ());
	_dlgData->SetCopyright (_copyright.GetString ());
	_dlgData->SetDefine (_define.IsChecked ());
	_dlgData->SetDefault (_default.IsChecked ());
	_dlgData->SetUseContent (_useCopyright.IsChecked ());

	if (_dlgData->IsNameOK ())
		EndOk ();
	else
		_dlgData->DisplayNameErrors ();
	return true;
}
