#include "Parameters.h"
#include "indentDetect.h"

#include <stdio.h>
#include <string.h>
#include <immintrin.h>
#include <assert.h>

typedef unsigned short u16;
typedef unsigned int u32;

union Vec_u16x8
{
	u16 len[8];
	void set(u16 val) { memset(len, 0, sizeof(*this)); }
	void adds(u16 v) { adds(v,v,v,v,v,v,v,v); }
	void adds(u16 a, u16 b, u16 c, u16 d, u16 e, u16 f, u16 g, u16 h) {
		__m128i vlen = _mm_loadu_si128((__m128i*)len);
		vlen = _mm_adds_epu16(vlen, _mm_setr_epi16(a,b,c,d,e,f,g,h)); 
		_mm_storeu_si128 ((__m128i*)len, vlen); }
	u16& operator[](size_t i) {	return len[i]; }
};


class IndentDetect_
{
public:
	inline void init(void);
	inline void eol(EolType& eolType);
	inline void indent(IndentStyle& indentStyle);
	inline void next(const char* buf, size_t size);
	
private:
	void line_init() { len.set(0); state = 0; flags = 0; }
	void line_end();

	void indent_end() {
		if(indentType_ > 0) indentType++;
		if(indentType_ < 0) indentType--;
	}
	
	void tab() { len.adds(1,2,3,4,5,6,7,8); }
	void spc() { len.adds(1,1,1,1,1,1,1,1); }
	static bool isNewLn(char ch) { return (ch == '\r')||(ch == '\n'); }
	static int isz(int i) { return (i&7)+1; }

	enum { FLAG_SPC=1, FLAG_TAB=2, FLAG_WEIRD=4,
		FLAG_LF=8, FLAG_CR=16, FLAG_BODY=32 };
	
	char state; char flags;
	int indent_count;
	int indentType_, indentType;
	int newLineType[3];
	Vec_u16x8 len; u16 prevLen[8];
	int prevIndent[16];
	int score[16][2];
};


void IndentDetect_::init(void)
{
	memset(this, 0, sizeof(*this));
}

void IndentDetect_::eol(EolType& eolType)
{
	line_end(); indent_end();
	
	u32 index = (u32)eolType;
	if(index < 3) newLineType[index] += 2;
	
	int best = 0;
	for(int i = 0; i < 3; i++) {
		if(best < newLineType[i]) {
			best = newLineType[i]; 
			eolType = (EolType)i; 
	}}
}


void IndentDetect_::indent(IndentStyle& indentStyle)
{
	line_end(); indent_end();
	
	// apply existing style
	indentType += indentStyle.useTabs ? 1 : -1;
	if(indentStyle.indentWidth != 0) {
		int index = indentStyle.indentWidth-1;
		if(indentStyle.tabWidth == 8) index += 8;
		score[index][1] += 1;
	}

	// calculate new style
	int best = -1; float bestScore = -1;		
	if(!indent_count) indent_count = 1;
	for(int i = 0; i < 16; i++) 
	{
		float x = float(score[i][0])/indent_count; 
		float y = float(score[i][1])/indent_count;
		float fscore = x + y*2;	
		
		//printf("%d: %f, %f, %d, %d, %d\n", isz(i), fscore, bestScore, indent_count, score[i][0], score[i][1]);
		
		if(isz(i) == 1) fscore *= 0.90F;
		fscore -= i*0.001F;
		if(bestScore < fscore) {
			bestScore = fscore; best = i; }
	}
	
	// apply new style
	indentStyle.useTabs = indentType > 0;
	indentStyle.indentWidth = isz(best);
	indentStyle.tabWidth = isz(best);
	if(best >= 8) {
		indentStyle.useTabs = false;
		indentStyle.tabWidth = 8; }
}


void IndentDetect_::next(const char* buf, size_t size)
{
	const char* end = buf + size; char ch;
	#define GETC(s) case s: if(buf >= end) { \
		state = s; return; } ch = *buf++;

	switch(state) {
	NEXT_LINE0: GETC(0);
	NEXT_LINE1:
		if(ch == '\t') {
	NEXT_TAB:
			flags |= FLAG_TAB;
			do { tab(); GETC(1);
			} while(ch == '\t');
		}
		
		if(ch == ' ') { 
			flags |= FLAG_SPC;
			do { spc(); GETC(2);
			} while(ch == ' ');
		}
		
		if(ch == '\t') {
			flags |= FLAG_WEIRD;
			goto NEXT_TAB;
		}
		
		while(!isNewLn(ch)) { flags |= FLAG_BODY; GETC(3); }
		if(ch == '\r') { flags |= FLAG_CR; GETC(4);
			if(ch != '\n') line_end(); goto NEXT_LINE1; }
		flags |= FLAG_LF; line_end(); goto NEXT_LINE0;
	}
}

void IndentDetect_::line_end()
{
	// line ending stats
	if(flags & (FLAG_LF|FLAG_CR)) {
		EolType index = EolType::unix;
		if(flags & FLAG_CR) { index = EolType::macos;
			if(flags & FLAG_LF) index = EolType::windows; }
		newLineType[(u32)index]++; 
	}
	
	if(!(flags & FLAG_WEIRD))
	{
		// tab/space
		if(flags & FLAG_TAB) indentType_++;
		else if(flags & FLAG_SPC) indentType_--;

		if(flags & FLAG_BODY) 
		{		
			// detect indent change
			bool changed = false;
			if(len[0]-prevLen[0] > 16) goto SKIP_LINE;
			for(int i = 0; i < 8; i++) {
				if((prevLen[i] == len[i])) continue;
				prevLen[i] = len[i]; changed = true;
			}
			
			if(changed) {
			
				// 
				indent_count++;
				for(int j = 0; j < 16; j++) { 
					int i = j > 7 ? 7 : j;
					int indent = len[i] / ((j&7)+1);
					if((len[i] % ((j&7)+1)) == 0) {
						if((indent - prevIndent[j]) != 1)
						score[j][0]++; else score[j][1]++;
						prevIndent[j] = indent; }
				}
				
				indent_end();
			}
		}
	}

SKIP_LINE:
	line_init();
}


void IndentDetect::init(IndentDetect* That)
{
	if(That) { this->move(That); }
	else { This = new IndentDetect_(); }
	This->init();
}

IndentDetect::~IndentDetect() 
{
	delete This; 
}

void IndentDetect::eol(EolType& eolType)
{
	This->eol(eolType);
}

bool IndentDetect::indent(IndentStyle& indentStyle)
{
	if(!This) return false;
	This->indent(indentStyle);
	return true;
}

void IndentDetect::next(const char* buf, size_t size) 
{
	This->next(buf, size); 
}
