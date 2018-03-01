#if !defined (SCCDLLFUNCTIONS_H)
#define SCCDLLFUNCTIONS_H
//------------------------------------
//  (c) Reliable Software, 1999 - 2008
//------------------------------------

#include "scc.h"

typedef LONG (*SccGetVersionPtr) (void);
typedef SCCRTN (*SccInitializePtr) (
									LPVOID * context, 
									HWND hWnd,		
									LPCSTR callerName,
									LPSTR sccName,	
									LPLONG sccCaps,
									LPSTR auxPathLabel,
									LPLONG checkoutCommentLen,
									LPLONG commentLen);
typedef SCCRTN (*SccUninitializePtr) (LPVOID context);
typedef SCCRTN (*SccOpenProjectPtr) (
									LPVOID context,
									HWND hWnd, 
									LPSTR lpUser,
									LPSTR lpProjName,
									LPCSTR lpLocalProjPath,
									LPSTR lpAuxProjPath,
									LPCSTR lpComment,
									LPTEXTOUTPROC lpTextOutProc,
									LONG dwFlags);
typedef SCCRTN (*SccCloseProjectPtr) (LPVOID context);
typedef SCCRTN (*SccGetProjPathPtr) (
									LPVOID context, 
									HWND hWnd, 
									LPSTR lpUser,
									LPSTR lpProjName, 
									LPSTR lpLocalPath,
									LPSTR lpAuxProjPath,
									BOOL bAllowChangePath,
									LPBOOL pbNew);
typedef SCCRTN (*SccGetPtr) (
									LPVOID context, 
									HWND hWnd, 
									LONG nFiles, 
									LPCSTR* lpFileNames, 
									LONG dwFlags,
									LPCMDOPTS pvOptions);
typedef SCCRTN (*SccCheckoutPtr) (
									LPVOID context, 
									HWND hWnd, 
									LONG nFiles, 
									LPCSTR* lpFileNames, 
									LPCSTR lpComment, 
									LONG dwFlags,
									LPCMDOPTS pvOptions);
typedef SCCRTN (*SccUncheckoutPtr) (
									LPVOID context, 
									HWND hWnd, 
									LONG nFiles, 
									LPCSTR* lpFileNames, 
									LONG dwFlags,
									LPCMDOPTS pvOptions);
typedef SCCRTN (*SccCheckinPtr) (
									LPVOID context, 
									HWND hWnd, 
									LONG nFiles, 
									LPCSTR* lpFileNames, 
									LPCSTR lpComment, 
									LONG dwFlags,
									LPCMDOPTS pvOptions);
typedef SCCRTN (*SccAddPtr) (
									LPVOID context, 
									HWND hWnd, 
									LONG nFiles, 
									LPCSTR* lpFileNames, 
									LPCSTR lpComment, 
									LONG * pdwFlags,
									LPCMDOPTS pvOptions);
typedef SCCRTN (*SccRemovePtr) (
									LPVOID context, 
									HWND hWnd, 
									LONG nFiles, 
									LPCSTR* lpFileNames,
									LPCSTR lpComment,
									LONG dwFlags,
									LPCMDOPTS pvOptions);
typedef SCCRTN (*SccRenamePtr) (
									LPVOID context, 
									HWND hWnd, 
									LPCSTR lpFileName,
									LPCSTR lpNewName);
typedef SCCRTN (*SccDiffPtr) (
									LPVOID context, 
									HWND hWnd, 
									LPCSTR lpFileName, 
									LONG dwFlags,
									LPCMDOPTS pvOptions);
typedef SCCRTN (*SccHistoryPtr) (
									LPVOID context, 
									HWND hWnd, 
									LONG nFiles, 
									LPCSTR* lpFileNames, 
									LONG dwFlags,
									LPCMDOPTS pvOptions);
typedef SCCRTN (*SccPropertiesPtr) (
									LPVOID context, 
									HWND hWnd, 
									LPCSTR lpFileName);
typedef SCCRTN (*SccQueryInfoPtr) (
									LPVOID context, 
									LONG nFiles, 
									LPCSTR* lpFileNames, 
									LPLONG lpStatus);
typedef SCCRTN (*SccPopulateListPtr) (
									LPVOID pContext, 
									enum SCCCOMMAND nCommand, 
									LONG nFiles, 
									LPCSTR* lpFileNames, 
									POPLISTFUNC pfnPopulate, 
									LPVOID pvCallerData,
									LPLONG lpStatus, 
									LONG dwFlags);
typedef SCCRTN (*SccGetEventsPtr) (
									LPVOID context, 
									LPSTR lpFileName,
									LPLONG lpStatus,
									LPLONG pnEventsRemaining);
typedef SCCRTN (*SccRunSccPtr) (
									LPVOID context, 
									HWND hWnd, 
									LONG nFiles, 
									LPCSTR* lpFileNames);
typedef SCCRTN (*SccGetCommandOptionsPtr) (
									LPVOID pContext, 
									HWND hWnd, 
									enum SCCCOMMAND nCommand,
									LPCMDOPTS * ppvOptions);
typedef SCCRTN (*SccAddFromSccPtr) (
									LPVOID pContext, 
									HWND hWnd, 
									LPLONG pnFiles,
									LPCSTR** lplpFileNames);
typedef SCCRTN (*SccSetOptionPtr) (
									LPVOID pContext,
									LONG nOption,
									LONG dwVal);
typedef long (*SccGetCoopVersionPtr) (void);

#endif
