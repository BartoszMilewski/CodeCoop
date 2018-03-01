//----------------------------
// (c) Reliable Software 1998
//----------------------------
#include "precompiled.h"
#include "CluLines.h"
#include <StringOp.h>
  
void Line::SetLen (size_t len)
{
	SimpleLine::SetLen (len);
	// A little heuristics
	if (Len () < 4)
	{
		if (Len () <= 2)
		{
			_similar.reserve (128);
		}
		else
		{
			_similar.reserve (64);
		}
	}
}

bool NullComparator::IsSimilar (Line const * line1, Line const * line2) const
{
	return false;
}

bool StrictComparator::IsSimilar (Line const * line1, Line const * line2) const
{
    int len = line1->Len ();
    if (len != line2->Len ())
        return false;
    return memcmp (line1->Buf (), line2->Buf (), len) == 0;
}

bool FuzzyComparator::IsSimilar (Line const * line1, Line const * line2) const
{
    int i1 = 0;
    int len1 = line1->Len ();
    char const * buf1 = line1->Buf ();
    int i2 = 0;
    int len2 = line2->Len ();
    char const * buf2 = line2->Buf ();
	// Do a quick check first
	if (len1 == len2 && memcmp (buf1, buf2, len1) == 0)
		return true;

	//	skip initial whitespace
    while (i1 < len1 && ::IsSpace (buf1[i1]))
		i1++;
    while (i2 < len2 && ::IsSpace (buf2[i2]))
        i2++;

    for (;;)
    {
        // break if either one exhausted
        if (i1 == len1 || i2 == len2)
            break;

        if (buf1 [i1] != buf2 [i2])
            return false;

        // they're equal so far
        i1++;
        i2++;

        if (i1 == len1 || i2 == len2)
            break;

		if (::IsSpace (buf1[i1]) && ::IsSpace (buf2[i2]))
		{
			//	both strings have a stretch of whitespace - eat it
			while (i1 < len1 && ::IsSpace (buf1[i1]))
				i1++;
			while (i2 < len2 && ::IsSpace (buf2[i2]))
				i2++;
		}
    }

	//	eat trailing whitespace
    while (i1 < len1 && ::IsSpace (buf1[i1]))
        i1++;
    while (i2 < len2 && ::IsSpace (buf2[i2]))
        i2++;

    return i1 == len1 && i2 == len2;
}
