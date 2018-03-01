#if !defined (TASKBARICON_H)
#define TASKBARICON_H
//----------------------------------
// (c) Reliable Software 1998 - 2006
// ---------------------------------
#include <Win/Message.h>
#include <Graph/Icon.h>

class TaskbarIcon
{
public:
	TaskbarIcon (Win::Dow::Handle winRecipient, int iconId, Icon::Handle image, Win::Message & msgNotify);
    ~TaskbarIcon ()
    {
        ::Shell_NotifyIcon (NIM_DELETE, &_iconData);
    }
	void ReShow ();
	void SetImage (Icon::Handle newImage)
    {
        _iconData.SetImage (newImage);
        ::Shell_NotifyIcon (NIM_MODIFY, &_iconData);
		_iconData.ResetFlags ();
    }
    void SetToolTip (char const * newToolTip)
    {
        _iconData.SetToolTip (newToolTip);
        ::Shell_NotifyIcon (NIM_MODIFY, &_iconData);
		_iconData.ResetFlags ();
    }
	void SetBalloonTip (char const * tip, char const * title)
	{
        _iconData.SetBalloonTip (tip, title);
        ::Shell_NotifyIcon (NIM_MODIFY, &_iconData);
		_iconData.ResetFlags ();
	}
	void ResetBalloonTip ()
	{
        _iconData.ResetBalloonTip ();
        ::Shell_NotifyIcon (NIM_MODIFY, &_iconData);
		_iconData.ResetFlags ();
	}
private:
    class NotifyIconData : public NOTIFYICONDATA
    {
    public:
        NotifyIconData (Win::Dow::Handle win);
		void ResetFlags () {	uFlags = 0;	}
		void SetMessage (unsigned int msgNotify)
		{
			uFlags |= NIF_MESSAGE;
			uCallbackMessage = msgNotify;
		}
		void SetImage (Icon::Handle newIcon)
        {
            uFlags |= NIF_ICON;
            hIcon = newIcon.ToNative ();
        }
        void SetToolTip (char const * newToolTip)
        {
            uFlags |= NIF_TIP;
			if (newToolTip == 0)
			{
				szTip [0] = 0;
			}
			else
			{
				if (_isPreVersion5)
					strncpy (szTip, newToolTip, 63);
				else
					strncpy (szTip, newToolTip, 127);
			}
        }
        void SetBalloonTip (char const * tip, char const * title)
        {
			if (_isPreVersion5)
				return;
            uFlags |= NIF_INFO;
			Assert (tip != 0);
			Assert (title != 0);
			strncpy (szInfo, tip, 255);
			strncpy (szInfoTitle, title, 63);
        }
		void ResetBalloonTip ()
		{
			if (_isPreVersion5)
				return;
			uFlags |= NIF_INFO;
			szInfo [0] = 0;
			szInfoTitle [0] = 0;
		}
	private:
		bool _isPreVersion5;
    };
private:
	Icon::Handle	_image;
	Win::Message	_notifyMsg;
	NotifyIconData	_iconData;
};

class TaskbarIconNotifyHandler
{
public:
	void OnTaskbarIconNotify (unsigned int iconId, long cmd);

protected:
	TaskbarIconNotifyHandler () {}

	virtual void OnLButtonDown    () {}
	virtual void OnRButtonDown    () {}
	virtual void OnRButtonUp      () {}
	virtual void OnLButtonDblClk  () {}
	virtual void OnKeySelect      () {}
	virtual void OnSelect         () {}
	virtual void OnContextMenu    () {}
	virtual void OnBalloonShow    () {}
	virtual void OnBalloonHide    () {}
	virtual void OnBalloonUserClk () {}
	virtual void OnBalloonTimeout () {}
};

#endif
