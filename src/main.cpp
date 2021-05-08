#include <ast.h>
#include <iostream>

#include <logger.h>

void print_ast(AST &ast)
{
	std::vector<Tok> out;
	ast.Print(out);
	for (auto &t : out)
		std::cout << t;
	std::cout << std::endl;
}

void print_lispy(AST &ast)
{
	std::vector<Tok> out;
	ast.PrintLispy(out);
	for (auto it = out.begin(); it != out.end(); ++it) {
		std::cout << *it;
		if (it->kind != Tok::Kind::END &&
		    it->kind != Tok::Kind::RBR_O &&
		    (it + 1)->kind != Tok::Kind::RBR_C)
			std::cout << " ";
	}
	std::cout << std::endl;
}

int main(int argc, char **argv)
{
	if (argc != 3)
		return 1;

	AST ast;
	AST diff;
	SymTabLoadFns();

	std::vector<Tok> tokvec;
	if (!LexTokenizeExpr(argv[1], tokvec))
		logger_fatal("tokenizer failed");

	if (!ast.FromExpr(tokvec))
		logger_fatal("parser failed");

	ast.FoldConst();
	std::cout << "opt:        ";
	print_ast(ast);

	if (!diff.FromDiff(ast, argv[2]))
		logger_fatal("diff. failed");
	diff.FoldConst();

	std::cout << "diff:       ";
	print_ast(diff);
	std::cout << "diff lispy: ";
	print_lispy(diff);
	return 0;
}