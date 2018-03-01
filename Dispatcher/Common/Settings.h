#if !defined (SETTINGS_H)
#define SETTINGS_H
//---------------------------------------
//  Settings.h
//  (c) Reliable Software, 1998, 99, 2000
//---------------------------------------

//
// View Registry Settings
//

class ViewSettings
{
public:
    ViewSettings (char const * viewName, 
				  const unsigned int defaultColWidths [],
				  const unsigned int colCount);
    void SaveSettings ();

    void SetColumnWidth (unsigned int col, int width);
    int  GetColumnWidth (unsigned int col) const;

	unsigned int GetSortCol () const { return _sortCol; }
	void SetSortCol (unsigned int col) {	_sortCol = col; }

private:
    char const *        _viewName;
	std::vector<int>	_width;
	unsigned int		_sortCol;
};

#endif
