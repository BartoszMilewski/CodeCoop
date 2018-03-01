//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "WikiController.h"
#include "resource.h"
#include "InPlaceBrowser.h"
#include "OutputSink.h"
#include "WikiConverter.h"
#include "LinkParser.h"
#include "Prompter.h"
#include "FeedbackMan.h"
#include "Sql.h"
#include "GlobalFileNames.h"
#include "Registry.h"

#include <Ctrl/Command.h>
#include <File/MemFile.h>
#include <Win/Controller.h>
#include <Com/Shell.h>
#include <fstream>

//------------------------
// Wiki Browser Controller
//------------------------

namespace Win
{
	class SimpleController: public Win::Controller
	{
	public:
		bool MustDestroy () throw () { return true; }
	};
}

WikiBrowserController::WikiBrowserController (int id,
					 Win::Dow::Handle win,
					 Cmd::Executor & executor,
					 Focus::Ring & focusRing)
	: _id (id),
	  _executor (executor),
	  _focusRing (focusRing),
	  _browser (0),
	  _feedback (0),
	  _scrollPos (0)
{
	Win::ChildMaker winMaker ("SimpleWinClass", win, id);
	std::unique_ptr<Win::Controller> ctrl (new Win::SimpleController);
	winMaker.Style () << Win::Style::Visible;
	_win = winMaker.Create (std::move(ctrl), "WebBrowserWindow");
	_webBrowser.reset (new InPlaceBrowser (_win));
	_webBrowser->SetEventHandler (this);
}

WikiBrowserController::~WikiBrowserController ()
{
	_executor.EnableKeyboardAccelerators ();
	if (!_redirPath.empty ())
		File::DeleteNoEx (_redirPath);
	_win.Destroy ();
}

WebBrowserView & WikiBrowserController::GetView () 
{
	return *_webBrowser.get ();
}

void WikiBrowserController::Activate (WikiBrowser * browser)
{
	_browser = browser;
	_browser->Attach (this, "browser");
}

void WikiBrowserController::StartNavigation (FeedbackManager & feedback)
{
	_feedback = &feedback;
	_wikiRoot.Change (_browser->GetCurrentDir ());
	FilePath tmpPath (_browser->GetTmpDir ());
	_globalDir.Change (_browser->GetGlobalDir ());
	_redirPath.assign (tmpPath.GetFilePath ("tmp.html"));
	NamedValues args;
	args.Add ("url", _wikiRoot.GetDir ());
	args.Add ("isStart", "true");
	_executor.PostCommand ("Navigate", args);
}

void WikiBrowserController::Navigate (std::string const & target, int scrollPos)
{
	_feedback->SetActivity ("Loading page");
	_scrollPos = scrollPos;
	_webBrowser->Navigate (target);
	_feedback->Close ();
	_webBrowser->SetFocus ();
}

void WikiBrowserController::ShowView ()
{
	Win::Accelerator * accel = _webBrowser->GetAccelHandler ();
	_executor.DisableKeyboardAccelerators (accel);
	_win.Show ();
}

void WikiBrowserController::HideView ()
{
	_executor.EnableKeyboardAccelerators ();
	_win.Hide ();
}

void WikiBrowserController::MoveView (Win::Rect const & viewRect)
{
	_win.Move (viewRect);
	Win::Rect rect;
	_win.GetClientRect (rect);
	_webBrowser->Move (rect);
}

// External notification
void WikiBrowserController::Update (std::string const & topic)
{
	if (topic == "browser" && !_srcUrl.empty ())
	{
		_scrollPos = _webBrowser->GetVScrollPos ();
		_webBrowser->Navigate (_srcUrl);
	}
}

// Feedback

void WikiBrowserController::DownloadBegin ()
{
	_feedback->SetActivity (_targetPath.c_str ());
}

void WikiBrowserController::DownloadComplete ()
{
	_feedback->SetActivity ("Done");
}

// progress = -1 means completion
void WikiBrowserController::ProgressChange (long progress, long progressMax)
{
	if (progress != -1 && progressMax != 0)
	{
		_feedback->SetRange (0, progressMax);
		_feedback->StepTo (progress);
	}
}

void WikiBrowserController::DocumentComplete (std::string const & url)
{
	_webBrowser->SetVScrollPos (_scrollPos);
	_scrollPos = 0;
	_feedback->Close ();
}

void WikiBrowserController::BeforeNavigate (std::string const & url, 
											Automation::Bool & cancel)
{
	int scrollPos = _webBrowser->GetVScrollPos ();
	bool forceReload = false;
	LinkParser link (url);
	if (link.IsWiki ())
	{
		try
		{
			std::string prevPath (_srcPath);
			std::string prevUrl (_srcUrl);
			_srcPath.assign (link.GetPath ());
			_srcUrl.assign ("file:///");
			_srcUrl += link.GetPath ();
			_srcUrl += link.GetQueryString ();
			_srcUrl += link.GetLabel ();
			_targetPath.assign (link.GetPath ());
			if (link.IsCreate ())
			{
				std::string const & nameSpace = link.GetNamespace (_wikiRoot.GetDir ());
				NewFile (nameSpace, true, false); // open it!, assume not system
				if (IsNocaseEqual (nameSpace, "Image"))
				{
					// don't navigate into image
					_srcPath.assign (prevPath);
					_srcUrl.assign (prevUrl);
				}
			}
			else if (link.IsDelete ())
			{
				DeleteFile ();
				_srcPath.assign (prevPath);
				_srcUrl.assign (prevUrl);
			}
			// create html file based on wiki file
			if (!Reload (link.GetQueryString (), link.GetLabel ()))
			{
				// Don't navigate into this page
				_srcPath.assign (prevPath);
				_srcUrl.assign (prevUrl);
				forceReload = true;
			}
		}
		catch (Win::Exception & e)
		{
			TheOutput.Display (e);
		}
		cancel.Set (true);
		// Notify the model that the path has changed
		NamedValues args;
		args.Add ("Path", _srcPath);
		args.Add ("ScrollPos", ToString (scrollPos));
		args.Add ("Url", _srcUrl);
		_executor.PostCommand ("OnBrowse", args);
		if (forceReload)
		{
			args.Clear ();
			_executor.PostCommand ("Selection_Reload", args);
		}
	}
	else if (!FilePath::IsEqualDir (link.GetPath (), _redirPath))
	{
#if 1
		ShellMan::Open (Win::Dow::Handle (), url.c_str ());
		cancel.Set (true);
#else
		_targetPath.assign (url);
		NamedValues args;
		args.Add ("Path", url);
		args.Add ("ScrollPos", ToString (scrollPos));
		args.Add ("Url", url);
		_executor.PostCommand ("OnBrowse", args);
#endif
	}
}

// Returns false if the file already exists
bool WikiBrowserController::NewFile (std::string const & nameSpace, bool doOpen, bool isSystemDir)
{
	if (File::Exists (_srcPath))
		return false;
	// execute new file command
	PathSplitter splitter (_srcPath);
	std::string fileName (splitter.GetFileName ());
	fileName += splitter.GetExtension ();
	std::string folderPath (splitter.GetDrive ());
	folderPath += splitter.GetDir ();
	ThePrompter.SetNamedValue ("FolderPath", folderPath);
	ThePrompter.SetNamedValue ("FileName", fileName);
	if (doOpen)
		ThePrompter.SetNamedValue ("Open", "true");
	ThePrompter.SetNamedValue ("Namespace", nameSpace);
	ThePrompter.SetNamedValue ("Add", isSystemDir? "false": "true");
	FilePath targetPath (folderPath);
	char const * templateFile = targetPath.GetFilePath ("template.wiki");
	if (File::Exists (templateFile))
	{
		MemFileReadOnly file (templateFile);
		std::string contents (file.GetBuf (), file.GetBuf () + file.GetBufSize ());
		ThePrompter.SetNamedValue ("Contents", contents);
	}
	_executor.ExecuteCommand ("DoNewFile");
	ThePrompter.ClearNamedValues ();
	return true;
}

void WikiBrowserController::DeleteFile ()
{
	ThePrompter.SetNamedValue ("FilePath", _srcPath);
	_executor.ExecuteCommand ("DoDeleteFile");
	ThePrompter.ClearNamedValues ();
}

// Returns false when must go to previous page
bool WikiBrowserController::Reload (std::string const & queryString, 
									std::string const & label)
{
	{
		// Create file. If existed, clear contents.
		File file (_redirPath, File::CreateAlwaysMode ());
	}
	{
		Sql::Listing::RecordFile recordFile (_srcPath);
		Sql::ExistingRecord record (_srcPath, recordFile);

		InStream wikiFile (_srcPath);
		OutStream htmlFile (_redirPath);
		LocalHtmlSink sink (htmlFile, _wikiRoot, _globalDir, queryString, record.GetTuples ());
		WikiConverter converter (wikiFile, sink);
		WikiPaths wikiPaths (_wikiRoot, _globalDir);
		// Parse wiki and create html file
		converter.ParseAndSave (wikiPaths.GetTemplPath (), wikiPaths.GetCssPath ());
		Sql::Command * cmd = converter.GetSqlCommand ();
		if (cmd != 0)
		{
			switch (cmd->GetType ())
			{
			case Sql::Command::Insert:
				{
					Sql::InsertCommand * insert = dynamic_cast<Sql::InsertCommand *> (cmd);
					Assert (insert != 0);
					if (cmd->GetTableName () == "REGISTRY")
					{
						InsertIntoRegistry (insert);
						return false;
					}
					else
					{
						if (DoInsert (insert))
						{
							NamedValues args;
							args.Add ("Path", _srcPath);
							_executor.PostCommand ("OpenFile", args);
						}
						_webBrowser->Navigate (_srcPath);
						return true;
					}
				}
				break;
			}
		}
	}
	std::string url ("file:///");
	url += _redirPath;
	url += queryString;
	url += label;
	_webBrowser->Navigate (url);
	return true;
}

// Returns false if the record already existed
bool WikiBrowserController::DoInsert (Sql::InsertCommand * insert)
{
	std::string const & tableName = insert->GetTableName ();
	// Create a new file
	FilePath newPath (insert->IsSystemTable ()?_globalDir: _wikiRoot);
	newPath.DirDown (tableName.c_str ());
	std::string fileName;
	Sql::TuplesMap & tuples = insert->GetTuples ();
	if (tuples.find (Sql::FileNamePseudoProp) != tuples.end ())
	{
		fileName.assign (tuples [Sql::FileNamePseudoProp]);
		tuples.erase (tuples.find (Sql::FileNamePseudoProp));
		unsigned extLen = strlen (".wiki");
		if (fileName.size () <= extLen
			|| !IsNocaseEqual (fileName.substr (fileName.size () - extLen), ".wiki"))
		{
			fileName += ".wiki";
		}
	}
	else
	{
		// Use the next available ID
		Sql::Listing table (_wikiRoot, 
							_globalDir, 
							tableName, 
							insert->IsSystemTable ());
		unsigned id = table.NextRecordId ();
		fileName.assign (ToString (id));
		fileName += ".wiki";
	}
	_srcPath = newPath.GetFilePath (fileName);
	_srcUrl = _srcPath;
	if (NewFile (tableName, false, insert->IsSystemTable ())) // don't open yet
	{
		Sql::WritableRecord record (_srcPath);
		record.Update (tuples);
		record.Save ();
		return true;
	}
	else
	{
		std::string fileQueryName = "ErrExists.wiki?filename=";
		fileQueryName += fileName;
		char const * fileQueryPath = _globalDir.GetFilePath (fileQueryName);
		_srcPath.assign (fileQueryPath);
		_srcUrl.assign ("file:///");
		_srcUrl += fileQueryPath;
		return false;
	}
}

void WikiBrowserController::InsertIntoRegistry (Sql::InsertCommand * insert)
{
	Assert (insert->GetTableName () == "REGISTRY");
	Sql::TuplesMap & tuples = insert->GetTuples ();
	Registry::UserWikiPrefs wikiReg;
	for (Sql::TuplesMap::const_iterator it = tuples.begin (); it != tuples.end (); ++it)
	{
		if (!it->first.empty ())
			wikiReg.SetPropertyValue (it->first, it->second);
	}
}

//---------------------
// Url Controller
//---------------------

UrlCtrl::UrlCtrl (int id,
				Win::Dow::Handle topWin,
				Win::Dow::Handle canvasWin,
				Cmd::Executor & executor)
	: Control::Handler (id),
	  _dropDown (topWin, id, canvasWin),
	  _executor (executor)
{
}

bool UrlCtrl::OnControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	// Notification from the drop down on the tool bar
	if (Win::DropDown::GotFocus (notifyCode))
		_executor.DisableKeyboardAccelerators (0);
	else if (Win::DropDown::LostFocus (notifyCode))
		_executor.EnableKeyboardAccelerators ();

	bool isSelection;
	if (_dropDown.OnNotify (notifyCode, isSelection))
	{
		std::string	url = _dropDown.RetrieveEditText ();
		if (!url.empty ())
		{
			EncodeUrl (url);
			ThePrompter.SetNamedValue ("url", url);
			_executor.ExecuteCommand ("Navigate");
			ThePrompter.ClearNamedValues ();
		}
	}
	return true;
}

void UrlCtrl::RefreshUrlList (std::string const & urls)
{
	// Filter can be a multi-line text.  Break it into
	// separate lines, display first line and add the rest
	// to the drop down list.
	unsigned int eolPos = urls.find_first_of ("\r\n");
	if (eolPos != std::string::npos)
	{
		std::string firstLine (&urls [0], eolPos);
		_dropDown.Display (firstLine.c_str ());
		do
		{
			unsigned int startPos = eolPos;
			while (IsEndOfLine (urls [startPos]))
				startPos++;
			eolPos = urls.find_first_of ("\r\n", startPos);
			int lineLen = (eolPos == std::string::npos) 
				? urls.length () - startPos 
				: eolPos - startPos;
			std::string line (&urls [startPos], lineLen);
			_dropDown.AddToList (line.c_str ());
		} while (eolPos != std::string::npos);
	}
	else
	{
		// Single line caption
		_dropDown.Display (urls.c_str ());
	}
	_dropDown.Show ();
}
