#ifndef INDENT_BAR_H
#define INDENT_BAR_H

union IndentStyle {
	struct {
		char indentWidth;
		char tabWidth;
		short useTabs;
	};

	int data;



	bool operator ==(int that) {
		return data == that; }
	operator int() { return data; }


	

	int fmtName(char* buff) const;
};

void identBar_create(HINSTANCE hInst, HWND hPere);
void identBar_addToRebar(ReBar * rebar);
void identBar_setStyle(IndentStyle style);

#endif
