#include "hjson.h"
#include <sstream>
#include <cmath>


namespace Hjson {


struct Parser {
  const unsigned char *data;
  size_t dataSize;
  int at;
  unsigned char ch;
};


static bool _parseFloat(double *pNumber, const std::string &str) {
  std::stringstream ss(str);

  // Make sure we expect dot (not comma) as decimal point.
  ss.imbue(std::locale::classic());

  ss >> *pNumber;

  return ss.eof() && !ss.fail() && !std::isinf(*pNumber) && !std::isnan(*pNumber);
}


static bool _next(Parser *p) {
  // get the next character.
  if (p->at < (int)p->dataSize) {
    p->ch = p->data[p->at++];
    return true;
  }

  if (p->at == (int)p->dataSize) {
    p->at++;
    p->ch = 0;
  }

  return false;
}


// Parse a number value.
bool tryParseNumber(double *pNumber, const char *text, size_t textSize, bool stopAtNext) {
  Parser p = {
    (const unsigned char*) text,
    textSize,
    0,
    ' '
  };

  int leadingZeros = 0;
  bool testLeading = true;

  _next(&p);

  if (p.ch == '-') {
    _next(&p);
  }

  while (p.ch >= '0' && p.ch <= '9') {
    if (testLeading) {
      if (p.ch == '0') {
        leadingZeros++;
      } else {
        testLeading = false;
      }
    }
    _next(&p);
  }

  if (testLeading) {
    leadingZeros--;
  } // single 0 is allowed

  if (p.ch == '.') {
    while (_next(&p) && p.ch >= '0' && p.ch <= '9') {
    }
  }
  if (p.ch == 'e' || p.ch == 'E') {
    _next(&p);
    if (p.ch == '-' || p.ch == '+') {
      _next(&p);
    }
    while (p.ch >= '0' && p.ch <= '9') {
      _next(&p);
    }
  }

  auto end = p.at;

  // skip white/to (newline)
  while (p.ch > 0 && p.ch <= ' ') {
    _next(&p);
  }

  if (stopAtNext) {
    // end scan if we find a punctuator character like ,}] or a comment
    if (p.ch == ',' || p.ch == '}' || p.ch == ']' ||
      p.ch == '#' || (p.ch == '/' && (p.data[p.at] == '/' || p.data[p.at] == '*')))
    {
      p.ch = 0;
    }
  }

  if (p.ch > 0 || leadingZeros != 0) {
    // Invalid number.
    return false;
  }

  return _parseFloat(pNumber, std::string((char*)p.data, end - 1));
}


bool startsWithNumber(const char *text, size_t textSize) {
  double number;
  return tryParseNumber(&number, text, textSize, true);
}


}
