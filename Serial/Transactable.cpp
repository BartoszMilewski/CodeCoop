//----------------------------------------
// (c) Reliable Software 1997
//----------------------------------------

#include "precompiled.h"
#include "Transactable.h"
#include <Ex/WinEx.h>

int TransactableSwitch::GetCurrent () const
{
    if (!IsValid (_value))
        throw Win::Exception ("Internal error: Corrupted transactable switch");
    return _value == OneValid ? 1 : 2;
}

int TransactableSwitch::XGetCurrent () const
{
    if (!IsValid (_xValue))
        throw Win::Exception ("Internal error: Corrupted transactable switch");
    return _xValue == OneValid ? 1 : 2;
}

int TransactableSwitch::GetPrevious () const
{
    if (!IsValid (_value))
        throw Win::Exception ("Internal error: Corrupted transactable switch");
    return _value == OneValid ? 2 : 1;
}

void TransactableSwitch::Deserialize (Deserializer& in, int version)
{ 
    _xValue = in.GetLong ();
    if (!IsValid (_xValue))
        throw Win::Exception ("Internal error: Database corrupted -- invalid original area id");
}

void TransactableSwitch::XSwitch ()
{
    _xValue = _xValue == OneValid ? TwoValid : OneValid;
}

bool TransactableSwitch::IsValid (int value) const
{
    return value == OneValid || value == TwoValid;
}
