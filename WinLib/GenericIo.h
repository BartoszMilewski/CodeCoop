#if !defined (GENERICIO_H)
#define GENERICIO_H
// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------

template <char endChar>
class GenericInput
{
public:
	virtual char Get () = 0; // returns endChar when called after end of data is reached
};

class GenericOutput
{
public:
	virtual void Put (char c) = 0;
	virtual void Flush () {} // client MUST call Flush when job is completed
};


#endif
