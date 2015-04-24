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
    else if (isalpha(p) ||
             (p == '-') || (p == '_') || (p == '*') || (p == '+')) {
      string id;
      cin.unget();
      cin >> id;
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

using BuiltinFunc =
    function<shared_ptr<SchemeType>(vector<shared_ptr<SchemeType> >)>;

//-----------------------------------------------------------------------------
class SchemeType {
 public:
  // Alternatively, should we use inheritance and polymorphism?
  enum class SexpType {
    ID, STR, INT, ERR, CONS, BUILTIN, NIL
  };

  SchemeType(int num) : ty_(SexpType::INT), num_(num) { }
  SchemeType(string id) : ty_(SexpType::ID), id_(id) { }
  SchemeType() : ty_(SexpType::ERR) { }
  SchemeType(SexpType ty) : ty_(ty) { }
  SchemeType(shared_ptr<SchemeType> car, shared_ptr<SchemeType> cdr) :
      ty_(SexpType::CONS), car_(car), cdr_(cdr) { }
  SchemeType(BuiltinFunc&& builtin) :
      ty_(SexpType::BUILTIN), builtin_(builtin) { }

  SexpType sexpType() { return ty_; }
  const string& id() { return id_; }
  int num() { return num_; }
  shared_ptr<SchemeType> car() { return car_; }
  shared_ptr<SchemeType> cdr() { return cdr_; }

  bool isNil() { return ty_ ==  SexpType::NIL; }

  void print(ostream& os);

 private:
  SexpType ty_;
  int num_;
  string id_;
  shared_ptr<SchemeType> car_;
  shared_ptr<SchemeType> cdr_;
  BuiltinFunc builtin_;
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
  shared_ptr<SchemeType> operator*() const { return cur_; }

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
    default:
      os << "*ERROR*";
      break;
  }
}

// TODO: this implementation assumes the last element is nil
vector<shared_ptr<SchemeType> > schemeListToVector(
    shared_ptr<SchemeType> sexp) {
  vector<shared_ptr<SchemeType> > vec;
  while (sexp->sexpType() == SchemeType::SexpType::CONS) {
    vec.push_back(sexp->car());
    sexp = sexp->cdr();
  }
}

shared_ptr<SchemeType> mapCar(
    function<shared_ptr<SchemeType>(shared_ptr<SchemeType>)> fn,
    shared_ptr<SchemeType> list) {
  if (list->isNil()) {
    return schemeNil;
  }
  else {
    return make_shared<SchemeType>(
        fn(list->car()),
        mapCar(fn, list->cdr()));
  }
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
        return analyzeApplication(sexp);
        break;
      default:
        return [sexp](Frame env) {
          return make_shared<SchemeType>(SchemeType::SexpType::ERR);
        };
    }
  }

  Expr analyzeApplication(shared_ptr<SchemeType> sexp) {
    Expr analyzedFunc = analyze(sexp->car());
    return analyzedFunc;
    /*
    // TODO: need to error if sexp->cdr() isn't a CONS
    Expr analyzedArgs =
        mapCar([](shared_ptr<SchemeType> i) { return analyze(i); }, sexp->cdr);
    return [analyzedFunc, analyzedArgs](shared_ptr<Frame> env) {
      shared_ptr<SchemeType> func = analyzedFunc(env);
      vector<shared_ptr<SchemeType>> args;
      std::
    };
    */
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
  auto expr = a.analyze(p.readSexp());
  expr(env)->print(cout);
  cout << endl;
}
