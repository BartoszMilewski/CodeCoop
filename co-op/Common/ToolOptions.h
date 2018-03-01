#if !defined (TOOLOPTIONS_H)
#define TOOLOPTIONS_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

namespace ToolOptions
{
	class Editor
	{
	public:
		Editor ();

		bool UsesExternalEditor () const { return _useExternalEditor; }

		void SetUseExternalEditor (bool flag)
		{
			_useExternalEditor = flag;
		}
		void SetExternalEditorPath (std::string const & path)
		{
			_externalEditorPath = path;
		}
		void SetExternalEditorCommand (std::string const & command)
		{
			_externalEditorCommand = command;
		}

		std::string const & GetExternalEditorPath () const { return _externalEditorPath; }
		std::string const & GetExternalEditorCommand () const { return _externalEditorCommand; }

	private:
		std::string	_externalEditorPath;
		std::string	_externalEditorCommand;
		bool		_useExternalEditor;
	};

	class Differ
	{
	public:
		Differ ();

		bool HasBc () const { return !_ourBcPath.empty () || !_originalBcPath.empty (); }
		bool HasOriginalBc () const { return !_originalBcPath.empty (); }
		bool HasGuiffy () const { return !_guiffyPath.empty (); }
		bool UsesGuiffy () const { return _useGuiffy; }
		bool UsesBc () const { return _useOriginalBc || _useOurBc; }
		bool UsesOriginalBc () const { return _useOriginalBc; }
		std::string const & GetOriginalBcPath () const { return _originalBcPath; }
		std::string const & GetGuiffyPath () const { return _guiffyPath; }
		std::string const & GetOurBcPath () const { return _ourBcPath; }

		void MakeUseDefault (bool val)
		{
			if (val)
			{
				MakeUseOurBc (false);
				MakeUseOriginalBc (false);
				MakeUseGuiffy (false);
			}
		}
		void MakeUseOriginalBc (bool val)
		{
			if (val)
			{
				MakeUseOurBc (false);
				MakeUseGuiffy (false);
			}
			if (_useOriginalBc != val)
			{
				_useOriginalBc = val;
			}
		}
		void MakeUseOurBc (bool val)
		{
			if (val)
			{
				MakeUseOriginalBc (false);
				MakeUseGuiffy (false);
			}
			if (_useOurBc != val)
			{
				_useOurBc = val;
			}
		}
		void MakeUseGuiffy (bool val)
		{
			if (val)
			{
				MakeUseOurBc (false);
				MakeUseOriginalBc (false);
			}
			if (_useGuiffy != val)
			{
				_useGuiffy = val;
			}
		}
	private:
		std::string	_ourBcPath;
		std::string	_originalBcPath;
		bool		_bcSupportsMerge;
		std::string	_guiffyPath;
		bool		_useOriginalBc;
		bool		_useOurBc;
		bool		_useGuiffy;
	};

	class Merger
	{
	public:
		Merger ();

		bool HasBc () const 
		{
			return !_ourBcPath.empty () 
				|| (!_originalBcPath.empty () && _bcSupportsMerge); 
		}
		bool BcSupportsMerge() const { return _bcSupportsMerge; }
		bool HasOriginalBc () const { return !_originalBcPath.empty (); }
		bool HasGuiffy () const { return !_guiffyPath.empty (); }
		bool UsesGuiffy () const { return _useGuiffy; }
		bool UsesBc () const { return _useOriginalBc || _useOurBc; }
		bool UsesOriginalBc () const { return _useOriginalBc; }
		std::string const & GetOriginalBcPath () const { return _originalBcPath; }
		std::string const & GetGuiffyPath () const { return _guiffyPath; }
		std::string const & GetOurBcPath () const { return _ourBcPath; }

		void MakeUseDefault (bool val)
		{
			if (val)
			{
				MakeUseOurBc (false);
				MakeUseOriginalBc (false);
				MakeUseGuiffy (false);
			}
		}
		void MakeUseOriginalBc (bool val)
		{
			if (val)
			{
				MakeUseOurBc (false);
				MakeUseGuiffy (false);
			}
			if (_useOriginalBc != val)
			{
				_useOriginalBc = val;
			}
		}
		void MakeUseOurBc (bool val)
		{
			if (val)
			{
				MakeUseOriginalBc (false);
				MakeUseGuiffy (false);
			}
			if (_useOurBc != val)
			{
				_useOurBc = val;
			}
		}
		void MakeUseGuiffy (bool val)
		{
			if (val)
			{
				MakeUseOurBc (false);
				MakeUseOriginalBc (false);
			}
			if (_useGuiffy != val)
			{
				_useGuiffy = val;
			}
		}
	private:
		std::string	_ourBcPath;
		bool		_bcSupportsMerge;
		std::string	_originalBcPath;
		std::string	_guiffyPath;
		bool		_useOriginalBc;
		bool		_useOurBc;
		bool		_useGuiffy;
	};
}

std::ostream & operator<<(std::ostream & os, ToolOptions::Editor const & editorOptions);
std::ostream & operator<<(std::ostream & os, ToolOptions::Differ const & differOptions);
std::ostream & operator<<(std::ostream & os, ToolOptions::Merger const & mergerOptions);

#endif
