#if !defined PARAMS_H
#define PARAMS_H
//----------------------------------
// (c) Reliable Software 1997 - 2005
//----------------------------------

// Global constants defining Code Co-op version

//----------------------------------------------------------------
// Model version defines the layout of Code Co-op Model class.
// Changing model version makes Code Co-op databases incompatibile
// and there is no going back to previous model version.
//
// BEFORE changing model version you need written permission from
// all project members.  If model version is changed without such
// permission you will be severly punished.
//
// YOU HAVE BEEN WARRNED !!!!!
//----------------------------------------------------------------

int  const modelVersion			= 55;

// Script version defines the layout of the Code Co-op script files.
// DO NOT CHANGE IT WITHOUT REALLY GOOD REASON !!
// Code Co-op has to read every script version ever defined !

// NOTICE: Changing the layout of FileData, Member, MemberDescription or MemberInfo
// requires changing both model and script versions to the same number

int const scriptVersion			= 45;

int const catalogVersion		= 54;

//----------------------------------------------------------------
// Model	Script	Change
//----------------------------------------------------------------
// 30		30
// 31		31		Add file type to aliases (FileData change!)
//			32		Add ScriptId to script headers
// 32				Add project properties bitset in the ProjectDb
// 33				Add admin bit to membership
// 34				Force admin election
// 35		35		Add CRCs
// 35		36		Added branch id and last received membership update script id to the script header
// 36		36		Cleaned up synch area (got rid of list of scripts, because we only allow one script in the synch area)
//					and fixed problem with membership update scripts (sender in his history had script with id x,
//					while scrip was send with id x+1), added last membership update id field to the history and
//					changed the way scripts are marked in the history note (file script always has inlineage bit set and
//					membership update has always this bit cleared).
// Version 4.0 released with model version == 36 and script version == 36
// 37               Added ScriptKind to the History::Note
// 38				Added oldest script id and most recent script id to the Member class
// 39				Added new History::Note::State bits
// 40				Replaced History::Db::_LMC with History::Db::_lastArchiveableIdx
// 41		38		Removed from the History::Note::State the following bits -- change script, defect, new user and next
//					Removed membership update placeholder scripts
//					Removed History::Note::_scriptKind
//					Added History::UnitNote and unit note list
//					Added History::Db::_nextScriptId
//					Matched script version with model version introducing oldest script id and most recent script id
//					to the Member class, so we can start exechanging membership update scripts
//           39		Added the following fileds to script header:
//						- script id -- lineage will no longer hold script id as the last item on the gid list
//						- unit type
//						- unit id
//						- side lineage list
//						- converted _LMC field into reserved field
//           40		Changed script kind to bit tree
//			 41		Combined version 4.0 file command types and control command types into one enumeration, so script
//					command list can contain both command kinds. Converted script header and command list into a
//					separate sections (in version 4.0 script header and command list formed one section).
// 42				Added to History::SortedTree _lastArchiveableIdx; removed it from History::Db
// 43				Added to History::Node _predecessorId, so we can walk history in the tree like fashion
// 44        44		64-bit file offsets in scripts
//           45		Chunking info in script header
// 45				Catalog: separate my hub's remote transport
// 46				Sidetrack added to Model
// 47				Sidetrack persists chunk info
// 48				History keeps the list of missing scripts from the future
// 49				Remember FS resend requests in sidetrack
// 50				History::SortedTree::_lastArchiveableIdx converted to _firstInterestingScriptId
// 51				Membership history tree cleanup after version 4.5b; Version 4.5c used this version
// 52				Added preprocessor flag needsProjectName to catalog
// 53				Added license (licensee & key) to catalog
// 54				Added distributor license to catalog
// 55				Membership history tree and project database cleanup in distribution project; Version 4.6a release

#endif
