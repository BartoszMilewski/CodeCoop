#if !defined (METRICS_H)
#define METRICS_H
//----------------------------------
// (c) Reliable Software 1998 - 2005
//----------------------------------

#include <Graph/Font.h>

class NonClientMetrics : public NONCLIENTMETRICS
{
public:
	NonClientMetrics ()
		: _metricsValid (true)
	{
		memset (this, 0, sizeof (NONCLIENTMETRICS));
		cbSize = sizeof (NONCLIENTMETRICS);
		if (!::SystemParametersInfo (SPI_GETNONCLIENTMETRICS, cbSize, this, 0))
			_metricsValid = false;
	}

	bool IsValid () const { return _metricsValid; }

	Font::Descriptor const & GetSmallCaptionFont () const 
	{
		return reinterpret_cast<Font::Descriptor const &> (lfSmCaptionFont);
	}
	Font::Descriptor const & GetMenuFont () const 
	{
		return reinterpret_cast<Font::Descriptor const &> (lfMenuFont);
	}
	Font::Descriptor const & GetStatusFont () const 
	{
		return reinterpret_cast<Font::Descriptor const &> (lfStatusFont);
	}
	Font::Descriptor const & GetMessageFont () const 
	{
		return reinterpret_cast<Font::Descriptor const &> (lfMessageFont);
	}
	int GetBorderWidth () const { return iBorderWidth; }
	int GetScrollWidth () const { return iScrollWidth; }
	int GetScrollHeigth () const { return iScrollHeight; }
	int GetCaptionHeight () const { return iCaptionHeight; }

private:
	bool _metricsValid;
};

#endif
