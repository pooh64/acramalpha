#include <string>
#include <iostream>
#include <unordered_map>
#include <cstring>
#include <cmath>
#include <cassert>

#include <ast.h>
#include <logger.h>

static std::unordered_map<char const *, Symbol> symtab;

Symbol *SymTabLookup(char const *id)
{
	auto it = symtab.find(id);
	if (it == symtab.end())
		return nullptr;
	return &it->second;
}

std::pair<Symbol *, bool> SymTabInstall(char const *id)
{
	auto [it, ins] = symtab.insert({ id, {} });
	if (ins) {
		it->second.str = id;
		it->second.kind = Symbol::Kind::VAR;
	}
	return { &it->second, ins };
}

Node *newFcallNode(Node *fn, Node *arg)
{
	Node *n = new Node;
	n->kind = Node::Kind::FCALL;
	n->fn = fn;
	n->arg = arg;
	return n;
}

Node *newUnaryNode(Tok::Kind uop, Node *next)
{
	Node *n = new Node;
	n->kind = Node::Kind::UNARY;
	n->uop = uop;
	n->next = next;
	return n;
}

Node *newBinaryNode(Tok::Kind bop, Node *l, Node *r)
{
	Node *n = new Node;
	n->kind = Node::Kind::BINARY;
	n->bop = bop;
	n->l = l;
	n->r = r;
	return n;
}

Node *newNumNode(float num)
{
	Node *n = new Node;
	n->kind = Node::Kind::NUM;
	n->num = num;
	return n;
}

Node *newSYMNode(char const *id)
{
	Node *n = new Node;
	auto [sym, ok] = SymTabInstall(id);
	n->kind = Node::Kind::SYM;
	n->sym = sym;
	return n;
}

Node *buildSymNode(char const *id)
{
	id = StrTabLookup(id);
	assert(id);
	Symbol *sym = SymTabLookup(id);
	assert(sym);
	Node *n = new Node;
	n->kind = Node::Kind::SYM;
	n->sym = sym;
	return n;
}

#define DEF_FUNC                                                               \
	DEF("exp", (float)exp(arg), nfn("exp", A))                             \
	DEF("ln", (float)log(arg), nbn(DIV, nnn(1), A))                        \
	DEF("sin", (float)sin(arg), nfn("cos", A))                             \
	DEF("cos", (float)cos(arg), nun(SUB, nfn("sin", A)))

void SymTabLoadFns()
{
	decltype(symtab)::iterator it;
	bool ins;
	Symbol s;
	std::string str;
	s.kind = Symbol::Kind::FUNC;

#define nnn(v) newNumNode(v)
#define nun(k, n) newUnaryNode(Tok::Kind::k, n)
#define nbn(k, l, r) newBinaryNode(Tok::Kind::k, l, r)
#define nfn(fnid, arg) newFcallNode(buildSymNode(fnid), arg)
#define A (arg->copyRecursive())
#define DEF(fname, eval, diff)                                                 \
	str = (fname);                                                         \
	s.str = StrTabInstall(str);                                            \
	s.evalfp = [](float arg) { return (float)(eval); };                    \
	s.difffp = [](Node *arg, char const *id) {                             \
		auto df = (diff);                                              \
		return nbn(MUL, df, arg->diffRecursive(id));                   \
	};                                                                     \
	std::tie(it, ins) = symtab.insert({ s.str, s });

	DEF_FUNC
#undef nnn
#undef nun
#undef nbn
#undef nfn
#undef A
#undef DEF
}

void Node::deleteRecursive()
{
	switch (kind) {
	case Node::Kind::NUM:
		break;
	case Node::Kind::SYM:
		break;
	case Node::Kind::UNARY:
		next->deleteRecursive();
		break;
	case Node::Kind::BINARY:
		l->deleteRecursive();
		r->deleteRecursive();
		break;
	case Node::Kind::FCALL:
		fn->deleteRecursive();
		arg->deleteRecursive();
		break;
	default:
		assert(0);
	}
	delete this;
}

// add:		mult | add '-' mult | add '+' mult
// mult:	unary | mult '*' unary | mult '/' unary
// unary:	factor | '+' unary | '-' unary
// factor:	SYM | NUM | '(' add ')' | factor '(' add ')'

std::tuple<Node *, bool> getAddExpr(std::vector<Tok>::iterator &cur);

std::tuple<Node *, bool> getFactorExpr(std::vector<Tok>::iterator &cur)
{
	auto k = cur->kind;
	Node *fn;
	if (k == Tok::Kind::NUM) {
		auto n = newNumNode(cur->num);
		cur++;
		fn = n;
	} else if (k == Tok::Kind::ID) {
		auto id = cur->id;
		cur++;
		fn = newSYMNode(id);
	} else if (k == Tok::Kind::RBR_O) {
		cur++;
		auto [e, ok] = getAddExpr(cur);
		if (!ok)
			return { nullptr, false };
		if (cur->kind != Tok::Kind::RBR_C) {
			e->deleteRecursive();
			return { nullptr, false };
		}
		cur++;
		fn = e;
	} else {
		return { nullptr, false };
	}

	if (cur->kind != Tok::Kind::RBR_O)
		return { fn, true };
	cur++;
	auto [e, ok] = getAddExpr(cur);
	if (!ok) {
		fn->deleteRecursive();
		return { nullptr, false };
	}
	if (cur->kind != Tok::Kind::RBR_C) {
		fn->deleteRecursive();
		e->deleteRecursive();
		return { nullptr, false };
	}
	cur++;

	// math restriction: fn must be SYM FUNC node:
	if (fn->kind != Node::Kind::SYM ||
	    fn->sym->kind != Symbol::Kind::FUNC) {
		fn->deleteRecursive();
		e->deleteRecursive();
		logger("only fn symbols are callable");
		return { nullptr, false };
	}
	return { newFcallNode(fn, e), true };
}

std::tuple<Node *, bool> getUnaryExpr(std::vector<Tok>::iterator &cur)
{
	Node *next;
	bool ok;

	auto k = cur->kind;
	if (k != Tok::Kind::ADD && k != Tok::Kind::SUB) {
		return getFactorExpr(cur);
	}
	cur++;

	std::tie(next, ok) = getUnaryExpr(cur);
	if (!ok) {
		return { nullptr, false };
	}
	return { newUnaryNode(k, next), true };
}

std::tuple<Node *, bool> getMultExpr(std::vector<Tok>::iterator &cur)
{
	Node *l, *r;
	bool ok;

	std::tie(l, ok) = getUnaryExpr(cur);
	if (!ok) {
		return { nullptr, false };
	}

	auto k = cur->kind;
	if (k != Tok::Kind::MUL && k != Tok::Kind::DIV) {
		return { l, true };
	}
	cur++;

	std::tie(r, ok) = getMultExpr(cur);
	if (!ok) {
		l->deleteRecursive();
		return { nullptr, false };
	}
	return { newBinaryNode(k, l, r), true };
}

std::tuple<Node *, bool> getAddExpr(std::vector<Tok>::iterator &cur)
{
	Node *l, *r;
	bool ok;

	std::tie(l, ok) = getMultExpr(cur);
	if (!ok) {
		return { nullptr, false };
	}

	auto k = cur->kind;
	if (k != Tok::Kind::ADD && k != Tok::Kind::SUB) {
		return { l, true };
	}
	cur++;

	std::tie(r, ok) = getAddExpr(cur);
	if (!ok) {
		l->deleteRecursive();
		return { nullptr, false };
	}
	return { newBinaryNode(k, l, r), true };
}

int printGetPrio(Node &n)
{
	if (n.kind == Node::Kind::UNARY)
		return 3;
	if (n.kind == Node::Kind::BINARY) {
		if (n.bop == Tok::Kind::MUL || n.bop == Tok::Kind::DIV)
			return 2;
		if (n.bop == Tok::Kind::ADD || n.bop == Tok::Kind::SUB)
			return 1;
		assert(0);
	}
	return 4;
}

void Node::printRecursive(std::vector<Tok> &out, int pprio)
{
	int cprio = printGetPrio(*this);
	if (cprio < pprio)
		out.push_back(Tok{ .kind = Tok::Kind::RBR_O });
	switch (kind) {
	case Kind::NUM:
		out.push_back(Tok{ .kind = Tok::Kind::NUM, .num = num });
		break;
	case Kind::SYM:
		out.push_back(Tok{ .kind = Tok::Kind::ID, .id = sym->str });
		break;
	case Kind::UNARY:
		out.push_back(Tok{ .kind = uop });
		next->printRecursive(out, cprio);
		break;
	case Kind::BINARY:
		l->printRecursive(out, cprio);
		out.push_back(Tok{ .kind = bop });
		r->printRecursive(out, cprio);
		break;
	case Kind::FCALL:
		fn->printRecursive(out, cprio);
		out.push_back(Tok{ .kind = Tok::Kind::RBR_O });
		arg->printRecursive(out, 0);
		out.push_back(Tok{ .kind = Tok::Kind::RBR_C });
		break;
	default:
		assert(0);
	}
	if (cprio < pprio)
		out.push_back(Tok{ .kind = Tok::Kind::RBR_C });
}

void Node::printLispy(std::vector<Tok> &out)
{
	switch (kind) {
	case Kind::NUM:
		out.push_back(Tok{ .kind = Tok::Kind::NUM, .num = num });
		return;
	case Kind::SYM:
		out.push_back(Tok{ .kind = Tok::Kind::ID, .id = sym->str });
		return;
	default:
		break;
	}

	out.push_back(Tok{ .kind = Tok::Kind::RBR_O });
	switch (kind) {
	case Kind::UNARY:
		out.push_back(Tok{ .kind = uop });
		next->printLispy(out);
		break;
	case Kind::BINARY:
		out.push_back(Tok{ .kind = bop });
		l->printLispy(out);
		r->printLispy(out);
		break;
	case Kind::FCALL:
		fn->printLispy(out);
		arg->printLispy(out);
		break;
	default:
		assert(0);
	}
	out.push_back(Tok{ .kind = Tok::Kind::RBR_C });
}

void AST::Print(std::vector<Tok> &out)
{
	root->printRecursive(out, 0);
	out.push_back({ .kind = Tok::Kind::END });
}

void AST::PrintLispy(std::vector<Tok> &out)
{
	root->printLispy(out);
	out.push_back({ .kind = Tok::Kind::END });
}

float Node::evalRecursive()
{
#define e(n) ((n)->evalRecursive())
#define DEF(tok, eval, diff)                                                   \
	case tok:                                                              \
		return eval;
	switch (kind) {
	case Kind::NUM:
		return num;
	case Kind::UNARY:
		switch (uop) {
			DEF_UNARY
		default:
			assert(0);
		}
	case Kind::BINARY:
		switch (uop) {
			DEF_BINARY
		default:
			assert(0);
		}
	case Kind::FCALL:
		return fn->sym->evalfp(arg->evalRecursive());
	default:
		assert(0);
	}
#undef e
#undef DEF
}

bool Node::isConstRecursive()
{
	switch (kind) {
	case Kind::SYM:
		return false;
	case Kind::NUM:
		return true;
	case Kind::UNARY:
		return next->isConstRecursive();
	case Kind::BINARY:
		return l->isConstRecursive() && r->isConstRecursive();
	case Kind::FCALL:
		return arg->isConstRecursive();
	default:
		assert(0);
	}
}

void AST::FoldConst()
{
	for (int i = 0; i < 10; ++i) {
		root->foldConstRecursive();
		root->optTrivialRecursive();
	}
}

bool Node::foldConstRecursive()
{
	bool cl, cr, cn;
	float tmp;
#define e(n) ((n)->num)
#define DEF(tok, eval, diff)                                                   \
	case tok:                                                              \
		tmp = eval;                                                    \
		break;

	switch (kind) {
	case Kind::SYM:
		return false;
	case Kind::NUM:
		return true;
	case Kind::UNARY:
		cn = next->foldConstRecursive();
		if (!cn)
			return false;
		switch (uop) {
			DEF_UNARY
		default:
			assert(0);
		}
		delete next;
		break;
	case Kind::BINARY:
		cl = l->foldConstRecursive();
		cr = r->foldConstRecursive();
		if (!cl || !cr)
			return false;
		switch (bop) {
			DEF_BINARY
		default:
			assert(0);
		}
		delete l;
		delete r;
		break;
	case Kind::FCALL:
		if (!arg->foldConstRecursive())
			return false;
		tmp = fn->sym->evalfp(arg->num);
		delete fn;
		delete arg;
		break;
	default:
		assert(0);
	}
	kind = Node::Kind::NUM;
	num = tmp;
	return true;
#undef DEF
}

Node *Node::copyRecursive()
{
	Node *c = new Node;
	*c = *this;

	switch (kind) {
	case Node::Kind::NUM:
		break;
	case Node::Kind::SYM:
		break;
	case Node::Kind::UNARY:
		c->next = next->copyRecursive();
		break;
	case Node::Kind::BINARY:
		c->l = l->copyRecursive();
		c->r = r->copyRecursive();
		break;
	case Node::Kind::FCALL:
		c->fn = fn->copyRecursive();
		c->arg = arg->copyRecursive();
		break;
	default:
		assert(0);
	}
	return c;
}

Node *Node::diffRecursive(char const *id)
{
#define nun(k, n) newUnaryNode(Tok::Kind::k, n)
#define nbn(k, l, r) newBinaryNode(Tok::Kind::k, l, r)
#define L (l->copyRecursive())
#define R (r->copyRecursive())
#define DN (next->diffRecursive(id))
#define DL (l->diffRecursive(id))
#define DR (r->diffRecursive(id))

#define DEF(tok, eval, diff)                                                   \
	case tok:                                                              \
		return diff;

	switch (kind) {
	case Node::Kind::NUM:
		return newNumNode(0);
	case Node::Kind::SYM:
		return newNumNode((id == sym->str) ? 1 : 0);
	case Node::Kind::UNARY:
		switch (uop) {
			DEF_UNARY
		default:
			assert(0);
		}
	case Node::Kind::BINARY:
		switch (bop) {
			DEF_BINARY
		default:
			assert(0);
		}
	case Node::Kind::FCALL:
		return fn->sym->difffp(arg, id);
	default:
		assert(0);
	}
#undef nun
#undef nbn
#undef L
#undef R
#undef DL
#undef DR
#undef DEF
}

bool AST::FromDiff(AST &src, char const *id)
{
	auto strid = std::string(id);
	id = StrTabInstall(strid);
	auto [sym, ok] = SymTabInstall(id);
	if (sym == nullptr || sym->kind != Symbol::Kind::VAR)
		return false;
	if (root)
		root->deleteRecursive();
	root = src.root->diffRecursive(id);
	return true;
}

bool AST::FromExpr(std::vector<Tok> &expr)
{
	auto it = expr.begin();
	auto [n, ok] = getAddExpr(it);
	if (!ok)
		return false;
	if (it->kind != Tok::Kind::END)
		return false;
	if (root)
		root->deleteRecursive();
	root = n;
	return true;
}

#define NodeIsNum(n) ((n)->kind == Node::Kind::NUM)
#define NodeIsVal(n, v) ((n)->kind == Node::Kind::NUM && (n)->num == (v))

void Node::optTrivialRecursive()
{
	switch (kind) {
	case Kind::UNARY:
		if (uop == Tok::Kind::ADD) {
			auto tmp = next;
			*this = *next;
			delete tmp;
			optTrivialRecursive();
			return;
		}
		next->optTrivialRecursive();
		break;
	case Kind::BINARY:
		if (NodeIsNum(r))
			std::swap(l, r);

		if (bop == Tok::Kind::MUL) {
			if (NodeIsVal(l, 0)) {
				l->deleteRecursive();
				r->deleteRecursive();
				kind = Node::Kind::NUM;
				num = 0;
				return;
			}
			if (NodeIsVal(l, 1)) {
				l->deleteRecursive();
				auto tmp = r;
				*this = *r;
				delete tmp;
				optTrivialRecursive();
				return;
			}
		}

		if (bop == Tok::Kind::ADD) {
			if (NodeIsVal(l, 0)) {
				l->deleteRecursive();
				auto tmp = r;
				*this = *r;
				delete tmp;
				optTrivialRecursive();
				return;
			}
		}
		l->optTrivialRecursive();
		r->optTrivialRecursive();
		break;
	case Kind::FCALL:
		arg->optTrivialRecursive();
		return;
	default:
		return;
	}
}