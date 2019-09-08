#include <stdexcept>
#include "ToolBar.h"
#include "indentBar.h"
#include "resource.h"

// WM_ERASEBKGND helper
static const RECT fillRC = { 0,0,32767,32767 };
static
void WINAPI FillWindow(HDC hdc, HBRUSH hBrush)
{
	FillRect(hdc, &fillRC, hBrush);
}


HWND WINAPI PossessWindow(HWND hwnd, WNDPROC wndProc, void* This)
{
	SendMessage(hwnd, WM_DESTROY, 0, 0);
	SendMessage(hwnd, WM_NCDESTROY, 0, 0);
	SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)wndProc);
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)This);
	wndProc(hwnd, WM_CREATE, 0, 0); return hwnd;
}

static HWND g_hIndentBar;
static HWND g_hIndentBar_;
static HWND g_hNppWindow;



IndentStyle defStyles[] = {
	{ 2, 2, true },{ 3, 3, true }, { 4, 4, true },{  8, 8, true },
	{ 2, 2, false},{ 3, 3, false },{ 4, 4, false },{  8, 8, false },
	{ 2, 8, true},{ 3, 8, true }, {  4, 8, true }
};


int IndentStyle::fmtName(char* str) const 
{
	if (indentWidth == tabWidth)
		return sprintf(str, useTabs ? "TAB-%d" : "SPC-%d", indentWidth);
	if ((tabWidth == 8) && (useTabs))
		return sprintf(str, "GNU-%d", indentWidth);
	return sprintf(str, useTabs ? "%d,%d-TAB" : 
		"%d,%d-SPC", indentWidth, tabWidth);
}

static
LRESULT CALLBACK wndProc(
	HWND   hwnd,
	UINT   uMsg,
	WPARAM wParam,
	LPARAM lParam
) {
	switch (uMsg) {
	case WM_ERASEBKGND:
		FillWindow((HDC)wParam, GetSysColorBrush(COLOR_3DFACE));
		return 0;
	case WM_COMMAND:
		if (HIWORD(wParam) == CBN_SELCHANGE) {
			int iSel = SendMessage(g_hIndentBar, CB_GETCURSEL, 0, 0);
			int data = SendMessage(g_hIndentBar, CB_GETITEMDATA, iSel, 0);
			SendMessage(g_hNppWindow, NPPM_INTERNAL_SETINDENTSTYLE, data, 0);
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


void identBar_create( HINSTANCE hInst, HWND hPere)
{
	// create ident bar
	g_hNppWindow = hPere;
	g_hIndentBar_ = CreateWindow(TEXT("STATIC"), TEXT(""),
		WS_CHILD | WS_VISIBLE,0, 0, 100, 20, hPere, NULL, hInst, NULL);
	PossessWindow(g_hIndentBar_, wndProc, 0);

	// create combobox
	g_hIndentBar = CreateWindow(WC_COMBOBOX, TEXT(""),
		CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE,
     0, 0, 100, 20, g_hIndentBar_, NULL, hInst, NULL);

	identBar_setStyle(defStyles[0]);
}

int identBar_addStyle(IndentStyle style)
{
	char buff[20]; style.fmtName(buff);
	int i = SendMessageA(g_hIndentBar, CB_ADDSTRING, 0, (LPARAM)buff);
	SendMessageA(g_hIndentBar, CB_SETITEMDATA, i, style.data);
	return i;
}

void identBar_setStyle(IndentStyle style)
{
	char buff[20];

	// setup the default styles
	SendMessage(g_hIndentBar, CB_RESETCONTENT, 0, 0);
	int iSel = -1;
	for (IndentStyle x : defStyles) {
		int i = identBar_addStyle(x); 
		if (style == x) iSel = i;
	}

	// set current style
	if (iSel < 0)
		iSel = identBar_addStyle(style);
	SendMessageA(g_hIndentBar, CB_SETCURSEL, iSel, 0);
}


void identBar_addToRebar(ReBar * rebar)
{
	REBARBANDINFO _rbBand = {0};
	_rbBand.cbSize = REBARBAND_SIZE;
	_rbBand.fMask = RBBIM_STYLE | RBBIM_CHILD
		| RBBIM_SIZE  | RBBIM_ID;

	_rbBand.fStyle		= RBBS_NOGRIPPER ;
	_rbBand.hwndChild	= g_hIndentBar_;
	_rbBand.wID			= REBAR_BAR_INDENT;	//ID REBAR_BAR_TOOLBAR for toolbar
	_rbBand.cx			= 50;

	rebar->addBand(&_rbBand, true);
	
}
