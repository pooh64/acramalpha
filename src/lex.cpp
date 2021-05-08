#include <vector>
#include <tuple>
#include <cstdlib>
#include <cerrno>
#include <cctype>
#include <ostream>
#include <cstring>
#include <assert.h>

#include <unordered_map>
#include <string>

#include <lex.h>

static std::unordered_map<std::string, char const*> strtab;

const char *StrTabLookup(char const *id)
{
	auto it = strtab.find(id);
	if (it == strtab.end())
		return nullptr;
	return it->second;
}

char const *StrTabInstall(std::string &str)
{
	auto [it, ins] = strtab.insert({ str, {} });
	if (ins) {
		it->second = it->first.c_str();
	}
	return it->second;
}

static bool getTokNum(char *&str, Tok &tok)
{
	if (!isdigit(*str))
		return false;
	char *endptr;
	errno = 0;
	float val = strtof(str, &endptr);
	if (endptr == str || errno)
		return false;
	str = endptr;
	tok.num = val;
	tok.kind = Tok::Kind::NUM;
	return true;
}

static bool getTokPunct(char *&str, Tok &tok)
{
	switch (*str) {
#define DEF(name, c)                                                           \
	case c:                                                                \
		tok.kind = Tok::Kind::name;                                    \
		break;
		DEF_TOK_PUNCT
#undef DEF
	default:
		return false;
	}
	str++;
	return true;
}

static bool getTokID(char *&str, Tok &tok)
{
	if (!isalpha(*str))
		return false;
	std::string buf;
	while (isalnum(*str))
		buf.push_back(*str++);
	auto id = StrTabInstall(buf);
	tok.kind = Tok::Kind::ID;
	tok.id = id;
	return true;
}

static std::tuple<Tok, bool> getTok(char *&str)
{
	Tok tok;
	if (getTokNum(str, tok))
		return { tok, true };
	if (getTokPunct(str, tok))
		return { tok, true };
	if (getTokID(str, tok))
		return { tok, true };
	return { tok, false };
}

bool LexTokenizeExpr(char *str, std::vector<Tok> &out)
{
	while (1) {
		while (isspace(*str))
			str++;
		auto [tok, ok] = getTok(str);
		if (!ok)
			break;
		out.push_back(tok);
	}
	out.push_back(Tok{ .kind = Tok::Kind::END });
	return !*str;
}

std::ostream &operator<<(std::ostream &os, const Tok &tok)
{
	switch (tok.kind) {
	case Tok::Kind::END:
		break;
	case Tok::Kind::NUM:
		os << tok.num;
		break;
	case Tok::Kind::ID:
		os << tok.id;
		break;
#define DEF(name, c)                                                           \
	case Tok::Kind::name:                                                  \
		os << c;                                                       \
		break;
		DEF_TOK_PUNCT
#undef DEF
	default:
		assert(0);
	}
	return os;
}