#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <memory>
#include <numeric>
#include <stack>
#include <tuple>
#include <vector>
#include <unordered_map>

using namespace std;

enum class TokenType : char {
  ID, STR, INT, BOOL, ERR, OP='(', CP=')', DOT='.', QUOTE='\''
};

class SchemeToken {
 public:
  SchemeToken(const TokenType ty) : ty_(ty) {}
  SchemeToken(int num) : ty_(TokenType::INT), num_(num) {}
  SchemeToken(string id) : ty_(TokenType::ID), id_(id) {}
  SchemeToken(bool b) : ty_(TokenType::BOOL), boolVal_(b) { }

  TokenType type() { return ty_; }
  int num() { return num_; }
  const string& id() { return id_; }
  string& take_id() { return id_; }
  bool boolVal() { return boolVal_; }

  TokenType ty_;
  int num_;
  string id_;
  bool boolVal_;
};

bool isSchemeId(char p) {
  return (isalpha(p) ||
          (p == '-') || (p == '_') || (p == '*') ||
          (p == '+') || (p == '?') || (p == '-') ||
          (p == '!'));
}

//-----------------------------------------------------------------------------
class Tokenizer {
 public:
  Tokenizer(istream& is) : is_(is) {}
  SchemeToken next();

  void unget(SchemeToken tok) {
    ungets_.push(std::move(tok));
  }

 private:
  istream& is_;
  stack<SchemeToken> ungets_;
};

SchemeToken Tokenizer::next() {
  if (!ungets_.empty()) {
    SchemeToken tmp = std::move(ungets_.top());
    ungets_.pop();
    return std::move(tmp);
  }

  for (;;) {
    char p = is_.get();
    if (isspace(p)) {
      continue;
    }
    else if (p == '#') {
      char bc = is_.get();
      if (bc == 't' || bc == 'f')
        return SchemeToken(bc == 't');
      else
        return SchemeToken(TokenType::ERR);
    }
    else if (isSchemeId(p)) {
      string id;
      do {
        id += p;
        p = is_.get();
      } while (is_.good() && (isalnum(p) || isSchemeId(p)));
      is_.unget();
      return SchemeToken(std::move(id));
    }
    else if (isdigit(p)) {
      int num;
      is_.unget();
      is_ >> num;
      return SchemeToken(num);
    }
    else if (p == '(' || p == ')' || p == '.' || p == '\'') {
      return SchemeToken((TokenType)p);
    }
    return SchemeToken(TokenType::ERR);
  }
}

class SchemeType;
class SchemeClosure;

// TODO: this is a shared_ptr due to a c++ compiler bug not present in clang;
//  seems like something to do with using SchemeType before definition.
//  Bummer that these have to be heap-allocated but oh well...
using BuiltinFunc = function<shared_ptr<SchemeType>(vector<SchemeType>&)>;

//-----------------------------------------------------------------------------
class SchemeType {
 public:
  // Alternatively, should we use inheritance and polymorphism?
  enum class SexpType {
    ID, INT, BOOL, ERR, CONS, BUILTIN, CLOSURE, NIL
  };

  SchemeType(int num) : ty_(SexpType::INT), num_(num) { }
  SchemeType(string id) : ty_(SexpType::ID), id_(id) { }
  SchemeType() : ty_(SexpType::ERR) { }
  SchemeType(SexpType ty) : ty_(ty) { }
  SchemeType(SchemeType car, SchemeType cdr) :
      ty_(SexpType::CONS),
      cons_(make_shared<pair<SchemeType, SchemeType>>(
          std::move(car), std::move(cdr))) { }
  SchemeType(BuiltinFunc&& builtin) :
      ty_(SexpType::BUILTIN), builtin_(builtin) { }
  SchemeType(shared_ptr<SchemeClosure> closure) :
    ty_(SexpType::CLOSURE), closure_(closure) { }

  static SchemeType fromBool(bool b) {
    SchemeType ret(SexpType::BOOL);
    ret.boolVal_ = b;
    return ret;
  }

  SexpType sexpType() { return ty_; }
  const string& id() const { return id_; }
  int num() { return num_; }
  bool boolVal() { return boolVal_; }
  SchemeType& car() { return cons_->first;  }
  SchemeType& cdr() { return cons_->second; }
  BuiltinFunc& builtin() { return builtin_; }
  shared_ptr<SchemeClosure> closure() { return closure_; }

  bool isNil() { return ty_ ==  SexpType::NIL; }

  bool toBool() {
    return (ty_ == SexpType::BOOL && boolVal_) ||
      (ty_ != SexpType::BOOL);
  }

  bool eq(SchemeType& other);

  void print(ostream& os);

 private:
  SexpType ty_;
  int num_;
  string id_;
  bool boolVal_;

  shared_ptr<pair<SchemeType, SchemeType> > cons_;
  BuiltinFunc builtin_;
  shared_ptr<SchemeClosure> closure_;
};

bool SchemeType::eq(SchemeType& other) {
  if (ty_ != other.ty_) return false;
  switch (ty_) {
  case SexpType::ID:
    // TODO use ATOMs
    return id_ == other.id_;
  case SexpType::INT:
    return num_ == other.num_;
  case SexpType::BOOL:
    return boolVal_ == other.boolVal_;
  case SexpType::CONS:
    return cons_ == other.cons_;
  case SexpType::BUILTIN:
    return true;  // TODO: this is a bug
    // OOPS NOT SUPPORTED! return builtin_ == other.builtin_;
  case SexpType::CLOSURE:
    return closure_ == other.closure_;
  case SexpType::ERR:
    return true;
  case SexpType::NIL:
    return true;
  }
}

SchemeType schemeNil = SchemeType::SexpType::NIL;

struct scheme_iterator {
  scheme_iterator(SchemeType& sexp) : cur_(&sexp) { }

  bool operator!=(scheme_iterator& other) const {
    // TODO: this is "good enough" but should be fixed
    return cur_->sexpType() != other.cur_->sexpType();
  }
  void operator++() { cur_ = &(cur_->cdr()); }
  SchemeType& operator*() const { return cur_->car(); }

  SchemeType* cur_;

  typedef forward_iterator_tag iterator_category;
  typedef shared_ptr<SchemeType> value_type;
  typedef int difference_type;
  typedef shared_ptr<SchemeType>* pointer;
  typedef shared_ptr<SchemeType>& reference;
};

scheme_iterator begin(SchemeType& sexp) {
  return scheme_iterator(sexp);
}

scheme_iterator end(SchemeType& sexp) {
  return schemeNil;
}

void printAll(SchemeType& sexp) {
  for (auto s : sexp) {
    s.print(cout);
    cout << endl;
  }
}

void SchemeType::print(ostream& os) {
  switch (ty_) {
  case SexpType::ID:
    os << id_;
    break;
  case SexpType::INT:
    os << num_;
    break;
  case SexpType::BOOL:
    os << '#' << (boolVal_ ? 't' : 'f');
    break;
  case SexpType::CONS: {
    // TODO!
    os << '(';
    cons_->first.print(os);
    SchemeType *rest = &(cons_->second);
    while (rest->ty_ == SexpType::CONS) {
      os << " ";
      rest->cons_->first.print(os);
      rest = &(rest->cons_->second);
    }
    if (rest->ty_ != SexpType::NIL) {
      os << " . ";
      rest->print(os);
    }
    os << ')';
    break;
  }
  case SexpType::NIL:
    os << "nil";
    break;
  case SexpType::BUILTIN:
    os << "*BUILTIN*";
    break;
  case SexpType::CLOSURE:
    os << "*CLOSURE*";
    break;
  default:
    os << "*ERROR*";
    break;
  }
}

ostream& operator<<(ostream& os, SchemeType& st) {
  st.print(os);
}

// TODO: this implementation assumes the last element is nil
vector<SchemeType> schemeListToVector(SchemeType& sexp) {
  vector<SchemeType> vec;
  std::copy(begin(sexp), end(sexp), back_inserter(vec));
  return vec;
}

SchemeType mapCar(function<SchemeType(SchemeType&)> fn,
                  SchemeType& list) {
  return std::accumulate(
      begin(list), end(list), schemeNil,
      [](SchemeType& a, SchemeType& i) {
        return SchemeType(i, a);
      });
}

//-----------------------------------------------------------------------------
class SchemeParser {
 public:
  SchemeParser(Tokenizer& tok) : tok_(tok) { }

  SchemeType readSexp();

 private:
  SchemeType readSexpList(bool allowDot);
  Tokenizer& tok_;
};

SchemeType SchemeParser::readSexp() {
  SchemeToken tok = tok_.next();
  if (tok.type() == TokenType::ERR) {
    return SchemeType();
  }
  else if (tok.type() == TokenType::INT) {
    return SchemeType(tok.num());
  }
  else if (tok.type() == TokenType::ID) {
    return SchemeType(tok.id());
  }
  else if (tok.type() == TokenType::BOOL) {
    return SchemeType::fromBool(tok.boolVal());
  }
  else if (tok.type() == TokenType::OP) {
    return readSexpList(false);
  }
  else if (tok.type() == TokenType::QUOTE) {
    return SchemeType(SchemeType("quote"),
                      SchemeType(readSexp(), schemeNil));
  }
}

SchemeType SchemeParser::readSexpList(bool allowDot) {
  SchemeToken tok = tok_.next();
  if (tok.type() == TokenType::DOT) {
    assert(allowDot);
    return readSexp();
  }
  else if (tok.type() == TokenType::CP) {
    return schemeNil;
  }
  else if (tok.type() == TokenType::OP) {
    SchemeType sCar(readSexpList(false));
    SchemeType sCdr(readSexpList(true));
    return SchemeType(std::move(sCar), std::move(sCdr));
  }
  else if (tok.type() == TokenType::INT ||
           tok.type() == TokenType::BOOL ||
           tok.type() == TokenType::ID ||
           tok.type() == TokenType::QUOTE) {
    tok_.unget(std::move(tok));
    SchemeType sexpForTok(readSexp());
    return SchemeType(std::move(sexpForTok), readSexpList(true));
  }
}

//-----------------------------------------------------------------------------
// TODO: should use interned atoms
using Symtab = unordered_map<string, SchemeType>;

class Frame : public Symtab,
              public enable_shared_from_this<Frame> {
 public:
  Frame(shared_ptr<Frame> next) : next_(next) { }
  shared_ptr<Frame> next() { return next_; }

  shared_ptr<Frame> findFrame(const string& sym) {
    shared_ptr<Frame> cur = shared_from_this();
    do {
      if (cur->find(sym) != cur->end()) {
        break;
      }
      else {
        cur = cur->next();
      }
    } while (cur);
    return cur;
  }

  SchemeType* lookup(const string& sym) {
    shared_ptr<Frame> frame = findFrame(sym);
    if (frame) {
      return &(*frame)[sym];
    }
    else {
      return nullptr;
    }
  }
 private:
  shared_ptr<Frame> next_;
};

//-----------------------------------------------------------------------------
struct SchemeClosure {
  shared_ptr<Frame> env_;
  vector<string> argNames_;
  function<SchemeType(shared_ptr<Frame>)> expr_;
};

//-----------------------------------------------------------------------------
bool carIsId(SchemeType& sexp, const string& id) {
  if (sexp.sexpType() == SchemeType::SexpType::CONS) {
    SchemeType& car = sexp.car();
    return (car.sexpType() == SchemeType::SexpType::ID
            && car.id() == id);
  }
  return false;
}

//-----------------------------------------------------------------------------
class SchemeAnalyzer {
 public:
  using Expr = function<SchemeType(shared_ptr<Frame>)>;
  Expr analyze(SchemeType& sexp) {
    switch (sexp.sexpType()) {
    case SchemeType::SexpType::INT:
    case SchemeType::SexpType::BOOL:
      return [sexp](shared_ptr<Frame> env) { return sexp; };
    case SchemeType::SexpType::ID:
      return [sexp](shared_ptr<Frame> env) {
        auto frame = env->findFrame(sexp.id());
        if (frame) {
          return (*frame)[sexp.id()];
        } else {
          return SchemeType(SchemeType::SexpType::ERR);
        }
      };
    case SchemeType::SexpType::CONS:
      if (carIsId(sexp, "lambda"))
        return analyzeLambda(sexp.cdr());
      else if (carIsId(sexp, "macroify"))
        return analyzeMacroify(sexp.cdr());
      else if (carIsId(sexp, "define"))
        return analyzeDefine(sexp.cdr());
      else if (carIsId(sexp, "quote"))
        return analyzeQuote(sexp.cdr());
      else if (carIsId(sexp, "and"))
        return analyzeAnd(sexp.cdr());
      else if (carIsId(sexp, "me")) {
        SchemeType s = sexp.cdr();
        return [this, s](shared_ptr<Frame> env) {
          SchemeType s2 = s;
          return expandMacros(s2);
        };
      }
      else
        return analyzeApplication(sexp);
      break;
    default:
      return [sexp](shared_ptr<Frame> env) {
        return SchemeType(SchemeType::SexpType::ERR);
      };
    }
  }

  unordered_map<string, shared_ptr<SchemeClosure> > macro_table_;

  SchemeType expandMacros_(SchemeType& sexp, bool* did_stuff) {
    if (sexp.sexpType() == SchemeType::SexpType::CONS) {
      if (sexp.car().sexpType() == SchemeType::SexpType::ID) {
        auto i = macro_table_.find(sexp.car().id());
        if (i != macro_table_.end()) {
          *did_stuff = true;
          auto sc = i->second;
          vector<SchemeType> args;
          std::copy(begin(sexp.cdr()), end(sexp.cdr()),
                    back_inserter(args));
          return applyClosure(sc, args);
        }
      }
      return SchemeType(expandMacros(sexp.car()),
                        expandMacros(sexp.cdr()));
    }
    else {
      return sexp;
    }
  }

  SchemeType expandMacros(SchemeType& sexp) {
    SchemeType s = sexp;
    bool did_stuff = false;
    do {
      s = expandMacros_(s, &did_stuff);
    }
    while(did_stuff);

    return s;
  }

  Expr analyzeMacroify(SchemeType& sexp) {
    string macro_name = sexp.car().id();
    return [this, macro_name](shared_ptr<Frame> env) {
      auto frame = env->findFrame(macro_name);
      if (frame) {
        auto sexp = ((*frame)[macro_name]);
        if (sexp.closure()) {
          macro_table_[macro_name] = sexp.closure();
          return SchemeType::fromBool(true);
        }
      }
      return SchemeType(SchemeType::SexpType::ERR);
    };
  }

  Expr analyzeAnd(SchemeType& sexp) {
    vector<Expr> exprs;
    std::transform(begin(sexp), end(sexp),
                   back_inserter(exprs),
                   [this](SchemeType& i) { return analyze(i); });
    return [exprs](shared_ptr<Frame> env) {
      SchemeType last;
      for (auto i : exprs) {
        last = i(env);
        if (!last.toBool()) {
          return SchemeType(SchemeType::fromBool(false));
        }
      }
      return last;
    };
  }

  Expr analyzeQuote(SchemeType& sexp) {
    SchemeType thing = sexp.car();
    return [thing](shared_ptr<Frame> env) {
      return thing;
    };
  }

  Expr analyzeDefine(SchemeType& sexp) {
    string id = sexp.car().id();
    Expr val = analyze(sexp.cdr().car());
    return [id, val](shared_ptr<Frame> env) {
      (*env)[id] = val(env);
      return schemeNil;
    };
  }

  Expr analyzeLambda(SchemeType& sexp) {
    // Extract the argument names -- those are in the car.
    vector<string> argNames;
    std::transform(begin(sexp.car()), end(sexp.car()),
                   back_inserter(argNames),
                   [](SchemeType& i) { return i.id(); });

    // Extract the argument body from the cdr.
    Expr body = analyzeBody(sexp.cdr());
    return [argNames, body](shared_ptr<Frame> env) {
      auto closure = make_shared<SchemeClosure>();
      closure->env_ = env;
      closure->argNames_ = std::move(argNames);
      closure->expr_ = body;
      return SchemeType(closure);
    };
  }

  Expr analyzeBody(SchemeType& sexpBody) {
    vector<Expr> exprs;
    std::transform(begin(sexpBody), end(sexpBody),
                   back_inserter(exprs),
                   [this](SchemeType& i) { return analyze(i); });
    return [exprs](shared_ptr<Frame> env) {
      SchemeType res;
      for (auto expr : exprs) {
        res = expr(env);
      }
      return res;
    };
  }

  SchemeType applyClosure(shared_ptr<SchemeClosure> closure,
                          vector<SchemeType>& eArgs) {
    // Create new environment frame.
    auto newEnv = make_shared<Frame>(closure->env_);

    int i = 0;
    // Bind arguments to values in the new frame
    for (const string& argName : closure->argNames_) {
      if (i >= eArgs.size()) {
        (*newEnv)[argName] = schemeNil;
      }
      else {
        (*newEnv)[argName] = eArgs[i];
      }
      i++;
    }
    return closure->expr_(newEnv);
  }

  Expr analyzeApplication(SchemeType& sexp) {
    Expr analyzedFunc = analyze(sexp.car());
    vector<Expr> analyzedArgs;
    std::transform(
      begin(sexp.cdr()), end(sexp.cdr()),
      back_inserter(analyzedArgs),
      [this](SchemeType& i) { return analyze(i); });
    return [this, analyzedFunc, analyzedArgs](shared_ptr<Frame> env) {
      auto eFunc = analyzedFunc(env);
      vector<SchemeType> eArgs;
      std::transform(
        begin(analyzedArgs), end(analyzedArgs),
        back_inserter(eArgs),
        [&env](Expr expr) { return expr(env); });
      if (eFunc.sexpType() == SchemeType::SexpType::BUILTIN) {
        // workaround for gnu compiler bug
        shared_ptr<SchemeType> unwrap_me = (eFunc.builtin())(eArgs);
        SchemeType cpy = *unwrap_me;
        return cpy;
      } else {
        assert(eFunc.sexpType() == SchemeType::SexpType::CLOSURE);
        auto closure = eFunc.closure();
        return applyClosure(closure, eArgs);
      }
    };
  }
};

//-----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
  Tokenizer t(cin);
  SchemeParser p(t);
  SchemeAnalyzer a;
  shared_ptr<Frame> env = make_shared<Frame>(nullptr);

  (*env)["x"] = SchemeType(999);
  (*env)["+"] = SchemeType(
      [](vector<SchemeType>& args) {
        return make_shared<SchemeType>(
            std::accumulate(args.begin(), args.end(), 0,
                            [](int a, SchemeType& b) {
                              return a + b.num();
                            }));
      });
  (*env)["eq?"] = SchemeType(
      [](vector<SchemeType>& args) {
        return make_shared<SchemeType>(
          SchemeType::fromBool(
            std::all_of((args.begin())++, args.end(),
                        [&args](SchemeType& i) {
                          return args[0].eq(i);
                        })));
      });
  (*env)["cons"] = SchemeType(
      [](vector<SchemeType>& args) {
        return make_shared<SchemeType>(args[0], args[1]);
      });
  (*env)["car"] = SchemeType(
      [](vector<SchemeType>& args) {
        return make_shared<SchemeType>(args[0].car());
      });
  (*env)["cdr"] = SchemeType(
      [](vector<SchemeType>& args) {
        return make_shared<SchemeType>(args[0].cdr());
      });

  while (cin.good()) {
    SchemeType sexp(p.readSexp());
    cout << "Evaluating: ";
    sexp.print(cout);
    cout << endl;
    auto expr = a.analyze(sexp);
    expr(env).print(cout);
    cout << endl;
  }
}
