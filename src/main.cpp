#include <ast.h>
#include <iostream>

void print_ast(AST &ast)
{
	std::vector<Tok> out;
	ast.Print(out);
	for (auto &t : out)
		std::cout << t;
	std::cout << std::endl;
}

int main(int argc, char **argv)
{
	if (argc != 3)
		return 1;

	bool ok;
	AST ast;
	AST diff;

	SymTabLoadFns();

	std::vector<Tok> tokvec;
	if (!LexTokenizeExpr(argv[1], tokvec))
		abort();

	ok = ast.FromExpr(tokvec);
	if (!ok)
		abort();
	ast.FoldConst();

	ok = diff.FromDiff(ast, argv[2]);
	if (!ok)
		abort();
	diff.FoldConst();

	print_ast(diff);
	return 0;
}