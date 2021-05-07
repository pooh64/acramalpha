#pragma once
#include <ostream>

#define DEF_TOK_PUNCT                                                          \
	DEF(ADD, '+')                                                          \
	DEF(SUB, '-')                                                          \
	DEF(MUL, '*')                                                          \
	DEF(DIV, '/')                                                          \
	DEF(RBR_O, '(')                                                        \
	DEF(RBR_C, ')')

char const *StrTabInstall(std::string &str);
char const *StrTabLookup(char const *id);

struct Tok {
	enum struct Kind {
		END,
		NUM,
		ID,
#define DEF(name, char) name,
		DEF_TOK_PUNCT
#undef DEF
	} kind;
	union {
		float num;
		char const *id;
	};
};

bool LexTokenizeExpr(char *str, std::vector<Tok> &out);
std::ostream &operator<<(std::ostream &os, const Tok &tok);