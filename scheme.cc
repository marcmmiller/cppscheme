#include <cassert>
#include <cctype>
#include <iostream>
#include <memory>
#include <stack>

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
      return SchemeToken(p);
    }
    return SchemeToken(TokenType::ERR);
  }
}

//-----------------------------------------------------------------------------
class SchemeType {
 public:
  enum class SexpType {
    ID, STR, INT, ERR, CONS, NIL
  };

  SchemeType(int num) : ty_(SexpType::INT), num_(num) { }
  SchemeType(string id) : ty_(SexpType::ID), id_(id) { }
  SchemeType() : ty_(SexpType::ERR) { }
  SchemeType(SexpType ty) : ty_(ty) { }
  SchemeType(shared_ptr<SchemeType> car, shared_ptr<SchemeType> cdr) :
      ty_(SexpType::CONS), car_(car), cdr_(cdr) { }

  void print(ostream& os);

 private:
  SexpType ty_;
  int num_;
  string id_;
  shared_ptr<SchemeType> car_;
  shared_ptr<SchemeType> cdr_;
};

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
    return make_shared<SchemeType>(SchemeType::SexpType::NIL);
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
int main(int argc, char* argv[]) {
  Tokenizer t(cin);
  cout << t.next().id_
       << endl
       << "done"
       << endl;
}
