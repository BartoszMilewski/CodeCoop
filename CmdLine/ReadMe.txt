Code Co-op Command Line Utility Notes
Copyright Reliable Software 2006 - 2010

Version 5.2

Syntax:

coopCmd:
	<root> | -p:<project id> -c:"All_Synch"
	
	<root> | -p:<project id> -c:"Project_Members userid:\"id\" [name:\"name\" | state:\"[voting/observer/removed/admin]]\""
	Note: to change the status of a local member enter: userid:\"-1\"

	<root> | -p:<project id> -c:"Project_Invite user:\"invitee name\" email:\"invitee email\" [observer:\"yes\"] [manualdispatch:\"yes\"]
			 [transferhistory:\"yes\"] [satellite:\"yes\" computer:\"name\"]
			 [target:"<invitation folder>"] [server:"<ftp server>"] [ftpuser:"<ftp user>"] [password:"<ftp password>"]

	<root> | -p:<project id> -c:"Selection_ChangeFileType type:\"text file\" \"c:\project\file.txt\""
	
	<root> | -p:<project id> -c:"All_Report view:[history | checkin | inbox | sync | project | file] target:\"file path\""

Notice: make sure to precede every internal quotation mark with a backslash. Project ID doesn't have to be quoted.

Example1: To invite Chad, who is an email peer, to the project Frequency Analyzer use:
		coopCmd "c:\work\Frequency Analyzer" -c:"Project_Invite user:\"Chad\" email:\"chad.codewell@gmail.com\" observer:\"yes\""
		
Example2: To invite Chad, who is an email peer, to the project Frequency Analyzer and store project invitation and history on the ftp site use:
		coopCmd "c:\work\Frequency Analyzer" -c:"Project_Invite user:\"Chad\" email:\"chad.codewell@gmail.com\" manualdispatch:\"yes\" transferhistory:\"yes\" target:"invitations" server:"ftp.codewell.com""

Example3: To re-send a membership script (only when instructed by support)
		coopCmd -p:8 -c:"Selection_SendScript script:\"1-3fe\" type:member recipientId:7"
Example4: To move a file from one folder to another
	coopCmd -p:8 -c:"Selection_Move \"c:\Project\foo.txt\" target:\"c:\Project\SubDir\""

allCoopCmd:
	"All_Synch"
	"Project_Members userid:\"-1\" [name:\"name\" | state:\"[voting/observer/removed/admin]]\""
	"Maintenance updatehubid:\"yes\""
	...
	
Notice: to operate on member states, set userid to -1
		
Example1: to synchronize all projects on a computer use:
		  allcoopcmd.exe All_Synch

Example2: to change status of a local member to observer use:
		  allcoopcmd.exe "Project_Members userid:\"-1\" state:\"observer\"" 
		
Example3: to invite Chad, who is an e-mail peer, as an Observer to all projects with Admin on this machine use:
		  allCoopCmd.exe "Project_Invite user:\"Chad\" email:\"chad.codewell@gmail.com\" observer:\"yes\""

Example4: after change of hub id (e-mail address) used by Dispatcher you can update all projects by executing command:
		  allCoopCmd.exe "Maintenance updatehubid:\"yes\""

