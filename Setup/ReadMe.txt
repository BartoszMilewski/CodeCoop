// ---------------------------
// (c) Reliable Software, 2003
// ---------------------------

To build a self-extracting installation of Co-op:
================================================

1. Build Co-op, CmdLine and Help solutions
2. Run copyd.bat or copyr.bat (d - debug, r - release)
3. Zip all files in bind (binr) folder.
4. Build self-extracting setup

To publish the installation exe on our website:
==============================================

1. Edit Version.xml file (Setup folder)
	To publish:
		- A new version of Co-op:
		    Edit:
				- <LatestVersion Number="xxx">
				- A headline sentence (the body of a <Description> tag)
				- make sure the exe name in href attribute is correct
		- A new beta version od Co-op:
				- same aas above, but in <LatestBeta> tag
		- New bulletin issue:
			Edit:
				- <Bulletin Number="xxx">
				- A headline sentence (the body of a <Bulletin> tag)
				
2. Upload installation exe and Version.xml files parallelly to two locations on our web site:
		ftp.relisoft.com
		and
		ftp.relisoft.com/www
  	    (you need to login as relisoft)

3. Optional steps:
	- Upload Bulletin4x.html to ftp.relisoft.com/www/co_op
	- Upload ReleaseNotes4x.txt to ftp.relisoft.com/www/co_op
