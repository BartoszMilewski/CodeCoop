#if !defined (NETSHAREIMPL_H)
#define NETSHAREIMPL_H
//---------------------------
// (c) Reliable Software 2000
//---------------------------
namespace Net
{
	class SharedObject;

	class ShareImpl
	{
	public:
		virtual void Add (SharedObject const & object) = 0;
		virtual void Delete (std::string const & netname) = 0;
	};
}

#endif
