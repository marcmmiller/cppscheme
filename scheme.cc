#include <cassert>
#include <cctype>
#include <iostream>
#include <memory>
#include <numeric>
#include <stack>
#include <vector>
#include <unordered_map>

using namespace std;

enum class TokenType : char {
  ID, STR, INT, ERR, OP='(', CP=')', DOT='.'
};

class SchemeToken {
 public:
  SchemeToken(const TokenType ty) : ty_(ty) {}
  SchemeToken(int num) : ty_(TokenType::INT), num_(num) {}
  SchemeToken(string id) : ty_(TokenType::ID), id_(id) {}

  TokenType type() { return ty_; }
  int num() { return num_; }
  const string& id() { return id_; }
  string& take_id() { return id_; }

  TokenType ty_;
  int num_;
  string id_;
};

bool isSchemeId(char p) {
  return (isalpha(p) ||
          (p == '-') || (p == '_') || (p == '*') || (p == '+'));
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
    else if (isSchemeId(p)) {
      string id;
      do {
        id += p;
        p = is_.get();
      } while (is_.good() && (isalnum(p) || isSchemeId(p)));
      cin.unget();
      return SchemeToken(std::move(id));
    }
    else if (isdigit(p)) {
      int num;
      cin.unget();
      cin >> num;
      return SchemeToken(num);
    }
    else if (p == '(' || p == ')' || p == '.') {
      return SchemeToken((TokenType)p);
    }
    return SchemeToken(TokenType::ERR);
  }
}

class SchemeType;
class SchemeClosure;

using BuiltinFunc =
    function<shared_ptr<SchemeType>(vector<shared_ptr<SchemeType> >)>;

//-----------------------------------------------------------------------------
class SchemeType {
 public:
  // Alternatively, should we use inheritance and polymorphism?
  enum class SexpType {
    ID, STR, INT, ERR, CONS, BUILTIN, CLOSURE, NIL
  };

  SchemeType(int num) : ty_(SexpType::INT), num_(num) { }
  SchemeType(string id) : ty_(SexpType::ID), id_(id) { }
  SchemeType() : ty_(SexpType::ERR) { }
  SchemeType(SexpType ty) : ty_(ty) { }
  SchemeType(shared_ptr<SchemeType> car, shared_ptr<SchemeType> cdr) :
      ty_(SexpType::CONS), car_(car), cdr_(cdr) { }
  SchemeType(BuiltinFunc&& builtin) :
      ty_(SexpType::BUILTIN), builtin_(builtin) { }
  SchemeType(shared_ptr<SchemeClosure> closure) :
    ty_(SexpType::CLOSURE), closure_(closure) { }

  SexpType sexpType() { return ty_; }
  const string& id() { return id_; }
  int num() { return num_; }
  shared_ptr<SchemeType> car() { return car_; }
  shared_ptr<SchemeType> cdr() { return cdr_; }
  BuiltinFunc& builtin() { return builtin_; }
  shared_ptr<SchemeClosure> closure() { return closure_; }

  bool isNil() { return ty_ ==  SexpType::NIL; }

  void print(ostream& os);

 private:
  SexpType ty_;
  int num_;
  string id_;
  shared_ptr<SchemeType> car_;
  shared_ptr<SchemeType> cdr_;
  BuiltinFunc builtin_;
  shared_ptr<SchemeClosure> closure_;
};

shared_ptr<SchemeType> schemeNil =
    make_shared<SchemeType>(SchemeType::SexpType::NIL);

struct scheme_iterator {
  scheme_iterator(shared_ptr<SchemeType> sexp) : cur_(sexp) { }

  bool operator!=(scheme_iterator& other) const {
    // TODO: this is "good enough" but should be fixed
    return cur_->sexpType() != other.cur_->sexpType();
  }
  void operator++() { cur_ = cur_->cdr(); }
  shared_ptr<SchemeType> operator*() const { return cur_->car(); }

  shared_ptr<SchemeType> cur_;

  typedef forward_iterator_tag iterator_category;
  typedef shared_ptr<SchemeType> value_type;
};

scheme_iterator begin(shared_ptr<SchemeType> sexp) {
  return scheme_iterator(sexp);
}

scheme_iterator end(shared_ptr<SchemeType> sexp) {
  return schemeNil;
}

void printAll(shared_ptr<SchemeType> sexp) {
  for (auto s : sexp) {
    s->print(cout);
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
    case SexpType::CONS:
      // TODO!
      os << '(';
      car_->print(os);
      os << " . ";
      cdr_->print(os);
      os << ')';
      break;
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

// TODO: this implementation assumes the last element is nil
vector<shared_ptr<SchemeType> > schemeListToVector(
    shared_ptr<SchemeType> sexp) {
  vector<shared_ptr<SchemeType> > vec;
  std::copy(begin(sexp->cdr()), end(sexp->cdr()),
            back_inserter(vec));
  return vec;
}

shared_ptr<SchemeType> mapCar(
    function<shared_ptr<SchemeType>(shared_ptr<SchemeType>)> fn,
    shared_ptr<SchemeType> list) {
  return std::accumulate(begin(list), end(list), schemeNil,
    [](shared_ptr<SchemeType> a, shared_ptr<SchemeType> i) {
      return make_shared<SchemeType>(i, a);
    });
}

//-----------------------------------------------------------------------------
class SchemeParser {
 public:
  SchemeParser(Tokenizer& tok) : tok_(tok) { }

  shared_ptr<SchemeType> readSexp();

 private:
  shared_ptr<SchemeType> readSexpList(bool allowDot);
  Tokenizer& tok_;
};

shared_ptr<SchemeType> SchemeParser::readSexp() {
  SchemeToken tok = tok_.next();
  if (tok.type() == TokenType::ERR) {
    return make_shared<SchemeType>();
  }
  else if (tok.type() == TokenType::INT) {
    return make_shared<SchemeType>(tok.num());
  }
  else if (tok.type() == TokenType::ID) {
    return make_shared<SchemeType>(tok.id());
  }
  else if (tok.type() == TokenType::OP) {
    return readSexpList(false);
  }
}

shared_ptr<SchemeType> SchemeParser::readSexpList(bool allowDot) {
  SchemeToken tok = tok_.next();
  if (tok.type() == TokenType::DOT) {
    assert(allowDot);
    return readSexp();
  }
  else if (tok.type() == TokenType::CP) {
    return schemeNil;
  }
  else if (tok.type() == TokenType::OP) {
    return make_shared<SchemeType>(
      readSexpList(false),
      readSexpList(true));
  }
  else if (tok.type() == TokenType::INT) {
    return make_shared<SchemeType>(
      make_shared<SchemeType>(tok.num()),
      readSexpList(true));
  }
  else if (tok.type() == TokenType::ID) {
    return make_shared<SchemeType>(
      make_shared<SchemeType>(tok.id()),
      readSexpList(true));
  }
}

//-----------------------------------------------------------------------------
// TODO: should use interned atoms
using Symtab = unordered_map<string, shared_ptr<SchemeType>>;

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
 private:
  shared_ptr<Frame> next_;
};

//-----------------------------------------------------------------------------
struct SchemeClosure {
  shared_ptr<Frame> env_;
  vector<string> argNames_;
  function<shared_ptr<SchemeType>(shared_ptr<Frame>)> expr_;
};

//-----------------------------------------------------------------------------
bool carIsId(shared_ptr<SchemeType>& sexp, const string& id) {
  if (sexp->sexpType() == SchemeType::SexpType::CONS) {
    shared_ptr<SchemeType> car = sexp->car();
    return (car->sexpType() == SchemeType::SexpType::ID
      && car->id() == id);
  }
  return false;
}

//-----------------------------------------------------------------------------
class SchemeAnalyzer {
 public:
  using Expr = function<shared_ptr<SchemeType>(shared_ptr<Frame>)>;
  Expr analyze(shared_ptr<SchemeType> sexp) {
    switch (sexp->sexpType()) {
      case SchemeType::SexpType::INT:
        return [sexp](shared_ptr<Frame> env) { return sexp; };
      case SchemeType::SexpType::ID:
        return [sexp](shared_ptr<Frame> env) {
          auto frame = env->findFrame(sexp->id());
          if (frame) {
            return (*frame)[sexp->id()];
          } else {
            return make_shared<SchemeType>(SchemeType::SexpType::ERR);
          }
        };
      case SchemeType::SexpType::CONS:
        if (carIsId(sexp, "lambda"))
          return analyzeLambda(sexp->cdr());
        else if (carIsId(sexp, "define"))
          return analyzeDefine(sexp->cdr());
        else
          return analyzeApplication(sexp);
        break;
      default:
        return [sexp](shared_ptr<Frame> env) {
          return make_shared<SchemeType>(SchemeType::SexpType::ERR);
        };
    }
  }

  Expr analyzeDefine(shared_ptr<SchemeType> sexp) {
    string id = sexp->car()->id();
    Expr val = analyze(sexp->cdr()->car());
    return [id, val](shared_ptr<Frame> env) {
      (*env)[id] = val(env);
      return schemeNil;
    };
  }

  Expr analyzeLambda(shared_ptr<SchemeType> sexp) {
    // Extract the argument names -- those are in the car.
    vector<string> argNames;
    std::transform(begin(sexp->car()), end(sexp->car()),
                   back_inserter(argNames),
                   [](shared_ptr<SchemeType> i) { return i->id(); });

    // Extract the argument body from the cdr.
    Expr body = analyzeBody(sexp->cdr());
    return [argNames, body](shared_ptr<Frame> env) {
      auto closure = make_shared<SchemeClosure>();
      closure->env_ = env;
      closure->argNames_ = std::move(argNames);
      closure->expr_ = body;
      return make_shared<SchemeType>(closure);
    };
  }

  Expr analyzeBody(shared_ptr<SchemeType> sexpBody) {
    vector<Expr> exprs;
    std::transform(begin(sexpBody), end(sexpBody),
                   back_inserter(exprs),
                   [this](shared_ptr<SchemeType> i) { return analyze(i); });
    return [exprs](shared_ptr<Frame> env) {
      shared_ptr<SchemeType> res;
      for (auto expr : exprs) {
        res = expr(env);
      }
      return res;
    };
  }

  Expr analyzeApplication(shared_ptr<SchemeType> sexp) {
    Expr analyzedFunc = analyze(sexp->car());
    vector<Expr> analyzedArgs;
    std::transform(
      begin(sexp->cdr()), end(sexp->cdr()),
      back_inserter(analyzedArgs),
      [this](shared_ptr<SchemeType> i) { return analyze(i); });
    return [this, analyzedFunc, analyzedArgs](shared_ptr<Frame> env) {
      auto eFunc = analyzedFunc(env);
      vector<shared_ptr<SchemeType>> eArgs;
      std::transform(
        begin(analyzedArgs), end(analyzedArgs),
        back_inserter(eArgs),
        [&env](Expr expr) { return expr(env); });
      if (eFunc->sexpType() == SchemeType::SexpType::BUILTIN) {
        return (eFunc->builtin())(eArgs);
      } else {
        assert(eFunc->sexpType() == SchemeType::SexpType::CLOSURE);
        auto closure = eFunc->closure();

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
    };
  }
};

//-----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
  Tokenizer t(cin);
  SchemeParser p(t);
  SchemeAnalyzer a;
  shared_ptr<Frame> env = make_shared<Frame>(nullptr);
  (*env)["x"] = make_shared<SchemeType>(999);
  (*env)["+"] = make_shared<SchemeType>(
      [](vector<shared_ptr<SchemeType> > args) {
        return make_shared<SchemeType>(
            std::accumulate(args.begin(), args.end(), 0,
                            [](int a, shared_ptr<SchemeType> b) {
                              return a + b->num();
                            }));
      });
  while (cin.good()) {
    shared_ptr<SchemeType> sexp = p.readSexp();
    cout << "Evaluating: ";
    sexp->print(cout);
    cout << endl;
    auto expr = a.analyze(sexp);
    expr(env)->print(cout);
    cout << endl;
  }
}
