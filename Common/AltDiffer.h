#if !defined (ALTDIFFER_H)
#define ALTDIFFER_H
// ----------------------------------
// (c) Reliable Software, 2004 - 2006
// ----------------------------------

std::string RetrieveFullBeyondComparePath (bool & supportsMerge);
std::string GetGuiffyPath ();
std::string GetAraxisPath ();
bool UsesAltDiffer (bool & useXml);
void SetUpOurAltDiffer (std::string const & exePath, bool quiet = false);

void SetupGuiffyRegistry (bool isOn, std::string const & exePath);
void SetupAraxisRegistry (bool isOn, std::string const & exePath);
bool UsesAltMerger (bool & useXml);

char const BCDIFFER_CMDLINE [] = "/lro /title1=\"$title1\" \"$file1\" \"$file2\"";
char const BCDIFFER_CMDLINE2 [] = "/lro /rro /title1=\"$title1\" /title2=\"$title2\" /vcsleft=\"$file3\" \"$file1\" \"$file2\"";
char const BCMERGER_CMDLINE [] = "/nobackups /ro1 /ro2 /ro3 /title1=\"$title1\" /title2=\"$title2\" /title3=\"$title3\" \"$file1\" \"$file2\" \"$file3\" \"$file4\"";
char const BCAUTOMERGER_CMDLINE [] = "/automerge /nobackups /ro1 /ro2 /ro3 \"$file1\" \"$file2\" \"$file3\" \"$file4\"";
char const GUIFFYDIFFER_CMDLINE [] = "\"-h1$title1\" \"$file1\" \"$file2\"";
char const GUIFFYDIFFER_CMDLINE2 [] = "\"-h1$title1\" \"-h2$title2\" \"$file1\" \"$file2\"";
char const GUIFFYMERGER_CMDLINE [] = "-s -h1\"$title1\" -h2\"$title2\" \"$file1\" \"$file2\" \"$file3\" \"$file4\"";
char const GUIFFYAUTOMERGER_CMDLINE [] = "-s \"$file1\" \"$file2\" \"$file3\" \"$file4\"";
char const ARAXISMERGER_CMDLINE [] = "/wait /3 /a3 /title1:\"$title1\" /title2:\"$title2\" /title3:\"Common Ancestor\" \"$file1\" \"$file2\" \"$file3\" \"$file4\"";
char const ARAXISAUTOMERGER_CMDLINE [] = "/3 /merge /a3 \"$file1\" \"$file2\" \"$file3\" \"$file4\"";

#endif
