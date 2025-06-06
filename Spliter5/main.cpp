#include "syntax.h"
#include<iostream>
int main() {
	Parser par;
	par.parsing("test.txt");
	DirSingleton::GetInstance().SetDir("C:\\Users\\È²±ÙÇÏ\\Desktop\\testfolder");
	SyntaxChecker sc(par.Getnodes(), par.GetError());
	sc.SyntaxCheck();
	sc.PrintErrors();

}