#include "syntax.h"
#include<iostream>
int main() {
	Parser par;
	par.parsing("test.txt");
	DirSingleton::GetInstance().SetDir("C:\\Users\\Ȳ����\\Desktop\\testfolder");
	SyntaxChecker sc(par.Getnodes(), par.GetError());
	sc.SyntaxCheck();
	sc.PrintErrors();

}