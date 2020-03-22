#pragma once


class IndentDetect_;
class IndentDetect
{
public:
	IndentDetect() : This(0) {}
	~IndentDetect();
	
	
	
	
	void init(IndentDetect* That);
	void move(IndentDetect* That) { 
		std::swap(That->This, This); }
	
	void eol(EolType& eolType);
	bool indent(IndentStyle& indentStyle);
	void next(const char* buf, size_t size);
	
private:
	IndentDetect_* This;
};
