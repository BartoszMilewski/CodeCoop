//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include "precompiled.h"
#include "ColumnInfo.h"

namespace Column
{
	const Info BrowsingFiles [] =
	{
		{ "File Name", Win::Report::Left, sortAscend, 30 },
		{ "State", Win::Report::Center,	sortAscend,	12 },
		{ "Type", Win::Report::Center, sortAscend, 12 },
		{ "Date Modified", Win::Report::Left, sortAscend, 24 },
		{ "Size", Win::Report::Right, sortAscend, 15 },
		{ "Global Id", Win::Report::Left, sortAscend, 12 },
		{ "", Win::Report::Left, sortNone, 0 }
	};

	const Info MiniProject [] = 
	{
		{"Project Name", Win::Report::Left, sortAscend, 30},
		{"Source Code Path", Win::Report::Left, sortAscend, 50},
		{"Last Modified", Win::Report::Left, sortDescend, 24 },
		{"Project Id", Win::Report::Right, sortAscend, 15},
		{ "", Win::Report::Left, sortNone, 0 }
	};

	const Info Checkin [] =
	{
		{"File Name", Win::Report::Left, sortAscend, 30},
		{"Change", Win::Report::Center, sortAscend, 12},
		{"State", Win::Report::Center, sortAscend, 12},
		{"Type", Win::Report::Center, sortAscend, 12},
		{"Date Modified", Win::Report::Left, sortAscend, 24},
		{"Path", Win::Report::Left, sortAscend, 50},
		{"Global Id", Win::Report::Left, sortAscend, 12},
		{ "", Win::Report::Left, sortNone, 0 }
	};

	const Info Synch [] =
	{
		{"File Name", Win::Report::Left, sortAscend, 30},
		{"Change", Win::Report::Center, sortAscend, 12},
		{"Type", Win::Report::Center, sortAscend, 12},
		{"Path", Win::Report::Left, sortAscend, 50},
		{"Global Id", Win::Report::Left, sortAscend, 12},
		{ "", Win::Report::Left, sortNone, 0 }
	};

	const Info Mailbox [] =
	{
		{"Script Name", Win::Report::Left, sortNone, 90},
		{"State", Win::Report::Left, sortNone, 15},
		{"From", Win::Report::Left, sortNone, 30},
		{"Date", Win::Report::Left, sortAscend, 34},
		{"Script Id", Win::Report::Left, sortNone, 12},
		{ "", Win::Report::Left, sortNone, 0 }
	};

	const Info History [] =
	{
		{"Version", Win::Report::Left, sortNone, 90},
		{"Created By", Win::Report::Left, sortNone, 20},
		{"Date", Win::Report::Left, sortAscend, 34},
		{"Script Id", Win::Report::Left, sortNone, 12},
		{"State", Win::Report::Left, sortNone, 15},
		{ "", Win::Report::Left, sortNone, 0 }
	};

	const Info Project [] =
	{
		{"Project Name", Win::Report::Left, sortAscend, 30},
		{"Source Code Path", Win::Report::Left, sortAscend, 50},
		{"Last Modified", Win::Report::Left, sortDescend, 24 },
		{"Project Id", Win::Report::Right, sortAscend, 15},
		{ "", Win::Report::Left, sortNone, 0 }
	};

	const Info NotInProject [] =
	{
		{" ", Win::Report::Left, sortNone, 100},
		{ "", Win::Report::Left, sortNone, 0 }
	};

	const Info ScriptDetails [] =
	{
		{"File Name", Win::Report::Left, sortAscend, 30},
		{"Change", Win::Report::Center, sortAscend, 12},
		{"Merge Status", Win::Report::Center, sortAscend, 18},
		{"Type", Win::Report::Center, sortAscend, 12},
		{"Path", Win::Report::Left, sortAscend, 50},
		{"Global Id", Win::Report::Left, sortAscend, 12},
		{ "", Win::Report::Left, sortNone, 0 }
	};

	const Info MergeDetails [] =
	{
		{"Source File Name", Win::Report::Left, sortAscend, 30},
		{"Merge Status", Win::Report::Center, sortAscend, 18},
		{"Source Path", Win::Report::Left, sortAscend, 40},
		{"Target Path", Win::Report::Left, sortAscend, 85},
		{"Global Id", Win::Report::Left, sortAscend, 12},
		{ "", Win::Report::Left, sortNone, 0 }
	};
}
