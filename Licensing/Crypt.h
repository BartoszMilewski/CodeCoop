#if !defined (CRYPT_H)
#define CRYPT_H
//----------------------------------
// Reliable Software (c) 1998 - 2007
//----------------------------------

std::string EncodeLicense10 (std::string const & licensee, int seats, char product);
bool DecodeLicense (std::string const & licenseString, int & version, int & seats, char & product);
bool IsCurrentVersion (int version);
bool IsValidProduct (char productId);

#endif
