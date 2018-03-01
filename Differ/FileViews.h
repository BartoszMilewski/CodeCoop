#if !defined (FILEVIEWS_H)
#define FILEVIEWS_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

enum FileSelection 
{
	FileCurrent,
	FileBefore, 
	FileAfter,
	FileMaxCount
};

// Note: modifying FileSelection enum requires modifying FileSelectionName array

extern char const * FileSelectionName [FileMaxCount];

class FileViewSelector
{
public:
	virtual void ChangeFileView (FileSelection page) = 0;
};

#endif
