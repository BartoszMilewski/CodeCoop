//---------------------------
// (c) Reliable Software 2006
//---------------------------
#include "precompiled.h"
#include "SqlHtmlSink.h"
#include "Sql.h"
#include "WikiDef.h"
#include "WikiConverter.h"
#include <File/Dir.h>

void SqlHtmlSink::SqlCmd (Sql::Command const & cmd)
{
	switch (cmd.GetType ())
	{
	case Sql::Command::Insert:
		{
			_out << "INSERT INTO " << cmd.GetTableName () << ":<br/>";
			Sql::InsertCommand const & insert = dynamic_cast<Sql::InsertCommand const &> (cmd);
			NocaseMap<std::string> const & tuples = insert.GetTuples ();
			_out << "<ul>" << std::endl;
			NocaseMap<std::string>::const_iterator it;
			for (it = tuples.begin (); it != tuples.end (); ++it)
			{
				_out << "<li>" << it->first << " = " << it->second << "</li>" << std::endl;
			}
			_out << "</ul>" << std::endl;
		}
		break;
	case Sql::Command::From:
		{
			std::string const & tableName = cmd.GetTableName ();
			if (IsNocaseEqual (tableName, "Image"))
			{
				ListImages (cmd.IsSystemTable ());
			}
			else
			{
				SqlFromCmd (dynamic_cast<Sql::FromCommand const &> (cmd));
			}
		}
		break;
	case Sql::Command::Select:
		SqlSelectCmd (dynamic_cast<Sql::SelectCommand const &> (cmd));
		break;
	}
#if 0
	// For testing only
	cmd.Dump (_out);
#endif
}

void SqlHtmlSink::ListImages (bool isSystem)
{
	FilePath path (isSystem? _sysPath: _curPath);
	path.DirDown ("Image");
	if (!File::Exists (path.GetDir ()))
	{
		_out << "No images<br/>" << std::endl;
		return;
	}
	FileMultiSeq::Patterns patterns;
	for (char const * const * pat = Wiki::ImagePattern; *pat != 0; ++pat)
		patterns.push_back (*pat);
	_out << "<ol>\n";
	for (FileMultiSeq dir (path, patterns); !dir.AtEnd (); dir.Advance ())
	{
		char const * name = dir.GetName ();
		std::string imageUrl;
		if (isSystem)
			imageUrl += "system:";
		imageUrl += "Image:";
		imageUrl += name;
		_out << "<li>" << name;
		if (!isSystem)
		{
			_out << ";&nbsp;(Delete: ";
			InternalDeleteLink (imageUrl);
			_out << ")";
		}
		_out << "<br/>\n";
		InternalLink (imageUrl, name);
		_out << "</li>\n";
	}
	_out << "</ol>\n";
}

void SqlHtmlSink::SqlSelectCmd (Sql::SelectCommand const & cmd)
{
	typedef std::vector<std::string> Strings;
	Strings const & selection = cmd.GetSelectFields ();
	Sql::Table table (_curPath, _sysPath, cmd);
	// Caption and headings
	_out << "\n<table>\n";
	_out << "    <tr>" << "<th>ID</th>";
	for (Strings::const_iterator it = selection.begin (); it != selection.end (); ++it)
	{
		if (Sql::IsPseudo (*it))
		{
			_out << "<th>" << Sql::DeCanonicalize (*it) << "</th>";
		}
		else
			_out << "<th>" << *it << "</th>";
	}
	_out << "</tr>" << std::endl;

	unsigned maxCount;
	Sql::Node const * predicate = cmd.GetPredicate ();
	if (predicate == 0 || !predicate->IsCount (maxCount))
		maxCount = 0xffffffff;
	unsigned count = 0;

	// Iterate over records in table
	for (Sql::Table::Iterator it = table.begin (); it != table.end () && count != maxCount; ++it, ++count)
	{
		Sql::Listing::RecordFile const * file = (*it)->GetRecordFile ();
		_out << "    <tr>" << std::endl;
		// ID
		std::string name = table.GetTableName ();
		name += ":";
		name += file->GetRecordName ();
		_out << "        <td>";

		if (cmd.IsSystemTable ())
		{
			std::string linkName (Wiki::SYSTEM_PREFIX);
			linkName += name;
			InternalLink (linkName, name);
		}
		else
			InternalLink (name, "");
		_out << "</td>" << std::endl;
		// Other fields
		for (Strings::const_iterator fi = selection.begin (); fi != selection.end (); ++fi)
		{
			_out << "        <td>";
			if (*fi == Sql::DeletePseudoProp)
			{
				InternalDeleteLink (name);
			}
			else
			{
				Sql::TuplesMap const & tuples = (*it)->GetTuples ();
				for (Sql::TuplesMap::const_iterator ti = tuples.begin (); ti != tuples.end (); ++ti)
				{
					if (IsNocaseEqual (*fi, ti->first))
					{
						// Property value may contain wiki formatting!
						std::stringstream in (ti->second);
						WikiParser parser (in, *this, false); // but don't use Sql!
						parser.Parse ();
						break;
					}
				}
			}
			_out << "</td>" << std::endl;
		}
		_out << "    </tr>" << std::endl;
	}
	_out << "\n</table>\n";
}

void SqlHtmlSink::SqlFromCmd (Sql::FromCommand const & cmd)
{
	Sql::Listing table (_curPath, _sysPath, cmd.GetTableName (), cmd.IsSystemTable ());
	table.DefaultSort ();
	_out << "\n<table>\n";
	_out << "<caption>" << table.GetTableName () << "</caption>" << std::endl;
	_out << "    <tr><th>ID</th></tr>" << std::endl;
	for (Sql::Table::RevFileIterator it = table.rbeginFile (); it != table.rendFile (); ++it)
	{
		_out << "    <tr>" << std::endl;
		std::string name = table.GetTableName ();
		name += ":";
		name += it->GetRecordName ();
		_out << "        <td>";
		InternalLink (name, it->GetRecordName ());
		_out << "</td>" << std::endl;
		_out << "    </tr>" << std::endl;
	}
	_out << "\n</table>\n";
}

