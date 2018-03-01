#if !defined (CONTROLHANDLER_H)
#define CONTROLHANDLER_H
//------------------------------
// (c) Reliable Software 2005
//------------------------------
 
namespace Control
{
	class Handler
	{
	public:
		explicit Handler (unsigned id)
			: _id (id)
		{}
		virtual ~Handler () {}

		bool IsHandlerFor (unsigned id) const
		{
			return id == _id;
		}
		unsigned GetId () const { return _id; }
		virtual bool OnControl (unsigned id, unsigned notifyCode) throw (Win::Exception) = 0;

	protected:
		unsigned	_id;
	};
}

#endif
