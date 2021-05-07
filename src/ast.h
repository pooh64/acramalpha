#pragma once

#include <tuple>
#include <vector>

#include <lex.h>

#define DEF_UNARY                                                              \
	DEF(Tok::Kind::SUB, -e(next), nun(SUB, DN))                            \
	DEF(Tok::Kind::ADD, e(next), DN)

#define DEF_BINARY                                                             \
	DEF(Tok::Kind::SUB, (e(l) - e(r)), nbn(SUB, DL, DR))                   \
	DEF(Tok::Kind::ADD, (e(l) + e(r)), nbn(ADD, DL, DR))                   \
	DEF(Tok::Kind::MUL, (e(l) * e(r)),                                     \
	    nbn(ADD, nbn(MUL, DL, R), nbn(MUL, L, DR)))                        \
	DEF(Tok::Kind::DIV, (e(l) / e(r)),                                     \
	    nbn(DIV, nbn(SUB, nbn(MUL, DL, R), nbn(MUL, L, DR)),               \
		nbn(MUL, R, R)))

struct Node;
struct Symbol {
	char const *str;
	enum struct Kind {
		VAR,
		FUNC,
	} kind;
	union {
		struct {
			float (*evalfp)(float);
			Node *(*difffp)(Node *, char const*);
		};
	};
};

struct Node {
	enum struct Kind {
		NUM,
		SYM,
		FCALL,
		UNARY,
		BINARY,
	} kind;
	union {
		float num;
		Symbol *sym;
		struct {
			Node *fn;
			Node *arg;
		};
		struct {
			Tok::Kind uop;
			Node *next;
		};
		struct {
			Tok::Kind bop;
			Node *l;
			Node *r;
		};
	};
	Node *copyRecursive();
	void deleteRecursive();
	void printRecursive(std::vector<Tok> &out, int pprio);
	float evalRecursive();
	bool isConstRecursive();
	bool foldConstRecursive();
	void optTrivialRecursive();
	Node *diffRecursive(const char *id);
};

struct AST {
	Node *root = nullptr;
	AST()
	{
	}
	AST(AST &&c)
	{
		root = c.root;
		c.root = nullptr;
	}
	AST(AST &c) = delete;
	~AST()
	{
		if (root)
			root->deleteRecursive();
	}
	bool FromDiff(AST &src, char const *id);
	bool FromExpr(std::vector<Tok> &expr);
	void Print(std::vector<Tok> &out);
	void FoldConst();
};

void SymTabLoadFns();