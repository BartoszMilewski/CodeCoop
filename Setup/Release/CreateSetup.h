#if !defined (CREATESETUP_H)
#define CREATESETUP_H
//---------------------------
// (c) Reliable Software 2010
//---------------------------
#include <File/Path.h>

char const coopProjName[] = "Rebecca";
char const webProjName[] = "CSS RSWeb";

enum ProductId {pro, lite, cmd, productCount};
typedef std::map<ProductId, std::string> ProdMap;

class ProductNames
{
public:
	static ProductId prodIds[productCount]; // for iteration
public:
	ProductNames();
	std::string const & Get(ProductId id) { return _productMap[id]; }
	std::string const & GetV(ProductId id) { return _productMapV[id]; }
	ProdMap::iterator beginId() { return _productMap.begin(); }
	ProdMap::iterator endId() { return _productMap.end(); }
private:
	ProdMap _productMap;
	ProdMap _productMapV; // with version number appended
};

// Product Names are used to create names for exe files
extern ProductNames TheProductNames;

class FileCopier
{
public:
	FileCopier(FilePath const & rootPath)
		: _rootPath(rootPath), _srcPath(rootPath), _zipFilePath(rootPath), _binrPath(rootPath)
	{
		_zipFilePath.DirDown("Setup\\Upload");
	}
	virtual void CopyFiles() = 0;
	void ZipFiles();
protected:
	virtual std::string CreateAboutText() = 0;
	void Copy(char const * srcDir, char const * file);
	void Copy(char const * file);

	std::string		_productName; // The name used for the setup exe
	FilePath const	_rootPath;
	FilePath		_zipFilePath;
	FilePath		_binrPath;
	std::string		_binDir;

	FilePath		_srcPath;
	FilePath		_tgtPath;
};

class CmdFileCopier: public FileCopier
{
public:
	CmdFileCopier(FilePath const & rootPath)
		: FileCopier(rootPath)
	{
		_binDir = "cmd";
		_binrPath.DirDown("CmdLine\\binr");
		_tgtPath = _binrPath;
		_productName = TheProductNames.Get(cmd);
	}
	void CopyFiles();
private:
	std::string CreateAboutText();
};

class CoopFileCopier: public FileCopier
{
public:
	CoopFileCopier(FilePath const & rootPath)
		: FileCopier(rootPath)
	{
		_binrPath.DirDown("Setup\\binr");
		_tgtPath = _binrPath;
	}
	void CopyFiles();
};

class ProFileCopier: public CoopFileCopier
{
public:
	ProFileCopier(FilePath const & rootPath)
		: CoopFileCopier(rootPath)
	{
		_binDir = "pro";
		_binrPath.DirDown(_binDir);
		_tgtPath.DirDown(_binDir);
		_productName = TheProductNames.Get(pro);
	}
	void CopyFiles();
private:
	std::string CreateAboutText();
};

class LiteFileCopier: public CoopFileCopier
{
public:
	LiteFileCopier(FilePath const & rootPath)
		: CoopFileCopier(rootPath)
	{
		_binDir = "lite";
		_binrPath.DirDown(_binDir);
		_tgtPath.DirDown(_binDir);
		_productName = TheProductNames.Get(lite);
	}
private:
	std::string CreateAboutText();
};

#endif
