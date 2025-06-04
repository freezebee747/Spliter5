#include "parser.h"
#include<iostream>
int main() {
	std::vector<std::string> test1 = { "rep1", "rep2" };
	std::string test2 = "$(test1)_$(test2)";

	std::cout << ReplaceVariable(test1, test2) << std::endl;

}