#if !defined (STOCKOBJ_H)
#define STOCKOBJ_H
// -----------------------------
// (c) Reliable Software, 2000
// -----------------------------
#include <Graph/Holder.h>
#include <Win/Handles.h>

namespace Stock
{
	// Stock::Object<BaseHandle>
	// Special case of Gdi::Handle
	template<class BaseHandle, class StockTypeEnum>
	class Object: public BaseHandle
	{
	public:
		explicit Object (StockTypeEnum type)
			: BaseHandle (reinterpret_cast<typename BaseHandle::Type> (
				::GetStockObject (type)))
		{}
		//operator Gdi::Handle () const { return Gdi::Handle (_h); }
	};

	// Stock::ObjectHolder
	template<class BaseHolder, class StockTypeEnum>
	class ObjectHolder: public BaseHolder
	{
	public:
		ObjectHolder (Win::Canvas canvas, StockTypeEnum type)
			: BaseHolder (canvas, Stock::Object<typename BaseHolder::Handle, StockTypeEnum> (type))
		{}
	};
}

#endif
