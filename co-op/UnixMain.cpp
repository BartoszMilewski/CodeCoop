//---------------------------
// (c) Reliable Software 2000
//---------------------------
#include "CoopMain.h"

#include <qapplication.h>

int main( int argc, char* argv[] )
{
  // PENDING(kalle) Check for system version? Probably not needed.

  // QApplication object; handles events, parses command-line parameters etc.
  QApplication coopapp( argc, argv );

  QString paramqstr;
  // build up paramstr as Windows does
  for( int i = 1; i < argc; i++ ) {
	paramqstr += argv[i];
	paramqstr += ' ';
  }
  // convert to char[] since we do not know whether it might be mutated
  char* paramstr = new char[ paramqstr.length() + 1 ];
  strcpy( paramstr, paramqstr.latin1() );
  paramstr[ paramqstr.length() ] = '\0';

  // turn over to platform-independent code
  int ret = CoopMain( &coopapp, paramstr, SW_SHOW );

  // cleanup
  delete paramstr;
  return ret;
}
