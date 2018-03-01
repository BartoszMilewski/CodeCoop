#if !defined (PATHCLASSIFIER_H)
#define PATHCLASSIFIER_H
//---------------------------------------------
// (c) Reliable Software 2004
//---------------------------------------------

class PathClassifier
{
public:
	PathClassifier (unsigned int pathCount, char const **paths);

	std::vector<int> const & GetProjectIds () const { return _projectIds; }
	std::vector<std::string> const & GetUnrecognizedPaths () const { return _unrecognizedPaths; }

private:
	std::vector<int>			_projectIds;
	std::vector<std::string>	_unrecognizedPaths;
};

#endif
