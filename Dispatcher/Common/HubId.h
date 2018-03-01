#if !defined (EMAILADDR_H)
#define EMAILADDR_H
//--------------------------------
// (c) Reliable Software 1998
// -------------------------------
#include <StringOp.h>

class HubId
{
public:
	HubId (std::string const & hubId) : _hubId (hubId) {}

	operator std::string const & () const { return _hubId; }

    bool operator < (HubId const & otherHubId) const
    {
        return IsNocaseLess (_hubId, otherHubId);
    }
    bool operator == (HubId const & otherHubId) const
    {
        return IsNocaseEqual (_hubId, otherHubId);
    }

private:
    std::string _hubId;
};

#endif
