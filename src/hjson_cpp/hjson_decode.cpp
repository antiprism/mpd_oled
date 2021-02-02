#include "hjson.h"
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstring>


namespace Hjson {


struct Parser {
  const unsigned char *data;
  size_t dataSize;
  int at;
  unsigned char ch;
};


bool tryParseNumber(double *pNumber, const char *text, size_t textSize, bool stopAtNext);
static Value _readValue(Parser *p);


// trim from start (in place)
static inline void _ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
    return !std::isspace(ch);
  }));
}


// trim from end (in place)
static inline void _rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
    return !std::isspace(ch);
  }).base(), s.end());
}


// trim from both ends (copy)
static inline std::string _trim(std::string s) {
  _rtrim(s);
  _ltrim(s);
  return s;
}


static void _resetAt(Parser *p) {
  p->at = 0;
  p->ch = ' ';
}


static bool _isPunctuatorChar(char c) {
  return c == '{' || c == '}' || c == '[' || c == ']' || c == ',' || c == ':';
}


static std::string _errAt(Parser *p, std::string message) {
  int i, col = 0, line = 1;

  for (i = p->at - 1; i > 0 && p->data[i] != '\n'; i--) {
    col++;
  }

  for (; i > 0; i--) {
    if (p->data[i] == '\n') {
      line++;
    }
  }

  size_t samEnd = std::min((size_t)20, p->dataSize - (p->at - col));

  return message + " at line " + std::to_string(line) + "," +
    std::to_string(col) + " >>> " + std::string((char*)p->data + p->at - col, samEnd);
}


static bool _next(Parser *p) {
  // get the next character.
  if (p->at < (int)p->dataSize) {
    p->ch = p->data[p->at++];
    return true;
  }

  p->ch = 0;

  return false;
}


static unsigned char _peek(Parser *p, int offs) {
  int pos = p->at + offs;

  if (pos >= 0 && pos < (int)p->dataSize) {
    return p->data[p->at + offs];
  }

  return 0;
}


static unsigned char _escapee(unsigned char c) {
  switch (c)
  {
  case '"':
  case '\'':
  case '\\':
  case '/':
    return c;
  case 'b':
    return '\b';
  case 'f':
    return '\f';
  case 'n':
    return '\n';
  case 'r':
    return '\r';
  case 't':
    return '\t';
  }

  return 0;
}


// Parse a multiline string value.
static std::string _readMLString(Parser *p) {
  std::vector<char> res;
  int triple = 0;

  // we are at ''' +1 - get indent
  int indent = 0;

  for (;;) {
    auto c = _peek(p, -indent - 5);
    if (c == 0 || c == '\n') {
      break;
    }
    indent++;
  }

  auto skipIndent = [&]() {
    auto skip = indent;
    while (p->ch > 0 && p->ch <= ' ' && p->ch != '\n' && skip > 0) {
      skip--;
      _next(p);
    }
  };

  // skip white/to (newline)
  while (p->ch > 0 && p->ch <= ' ' && p->ch != '\n') {
    _next(p);
  }
  if (p->ch == '\n') {
    _next(p);
    skipIndent();
  }

  // When parsing multiline string values, we must look for ' characters.
  bool lastLf = false;
  for (;;) {
    if (p->ch == 0) {
      throw syntax_error(_errAt(p, "Bad multiline string"));
    } else if (p->ch == '\'') {
      triple++;
      _next(p);
      if (triple == 3) {
        auto sres = res.data();
        if (lastLf) {
          return std::string(sres, res.size() - 1); // remove last EOL
        }
        return std::string(sres, res.size());
      }
      continue;
    } else {
      while (triple > 0) {
        res.push_back('\'');
        triple--;
        lastLf = false;
      }
    }
    if (p->ch == '\n') {
      res.push_back('\n');
      lastLf = true;
      _next(p);
      skipIndent();
    } else {
      if (p->ch != '\r') {
        res.push_back(p->ch);
        lastLf = false;
      }
      _next(p);
    }
  }
}


static void _toUtf8(std::vector<char> &res, uint32_t uIn) {
  if (uIn < 0x80) {
    res.push_back(uIn);
  } else if (uIn < 0x800) {
    res.push_back(0xc0 | ((uIn >> 6) & 0x1f));
    res.push_back(0x80 | (uIn & 0x3f));
  } else if (uIn < 0x10000) {
    res.push_back(0xe0 | ((uIn >> 12) & 0xf));
    res.push_back(0x80 | ((uIn >> 6) & 0x3f));
    res.push_back(0x80 | (uIn & 0x3f));
  } else if (uIn < 0x110000) {
    res.push_back(0xf0 | ((uIn >> 18) & 0x7));
    res.push_back(0x80 | ((uIn >> 12) & 0x3f));
    res.push_back(0x80 | ((uIn >> 6) & 0x3f));
    res.push_back(0x80 | (uIn & 0x3f));
  } else {
    throw std::logic_error("Invalid unicode code point");
  }
}


// Parse a string value.
// callers make sure that (ch === '"' || ch === "'")
// When parsing for string values, we must look for " and \ characters.
static std::string _readString(Parser *p, bool allowML) {
  std::vector<char> res;

  char exitCh = p->ch;
  while (_next(p)) {
    if (p->ch == exitCh) {
      _next(p);
      if (allowML && exitCh == '\'' && p->ch == '\'' && res.size() == 0) {
        // ''' indicates a multiline string
        _next(p);
        return _readMLString(p);
      } else {
        return std::string(res.data(), res.size());
      }
    }
    if (p->ch == '\\') {
      unsigned char ech;
      _next(p);
      if (p->ch == 'u') {
        uint32_t uffff = 0;
        for (int i = 0; i < 4; i++) {
          _next(p);
          unsigned char hex;
          if (p->ch >= '0' && p->ch <= '9') {
            hex = p->ch - '0';
          } else if (p->ch >= 'a' && p->ch <= 'f') {
            hex = p->ch - 'a' + 0xa;
          } else if (p->ch >= 'A' && p->ch <= 'F') {
            hex = p->ch - 'A' + 0xa;
          } else {
            throw syntax_error(_errAt(p, std::string("Bad \\u char ") + (char)p->ch));
          }
          uffff = uffff * 16 + hex;
        }
        _toUtf8(res, uffff);
      } else if ((ech = _escapee(p->ch))) {
        res.push_back(ech);
      } else {
        throw syntax_error(_errAt(p, std::string("Bad escape \\") + (char)p->ch));
      }
    } else if (p->ch == '\n' || p->ch == '\r') {
      throw syntax_error(_errAt(p, "Bad string containing newline"));
    } else {
      res.push_back(p->ch);
    }
  }

  throw syntax_error(_errAt(p, "Bad string"));
}


// quotes for keys are optional in Hjson
// unless they include {}[],: or whitespace.
static std::string _readKeyname(Parser *p) {
  if (p->ch == '"' || p->ch == '\'') {
    return _readString(p, false);
  }

  std::vector<char> name;
  auto start = p->at;
  int space = -1;
  for (;;) {
    if (p->ch == ':') {
      if (name.empty()) {
        throw syntax_error(_errAt(p, "Found ':' but no key name (for an empty key name use quotes)"));
      } else if (space >= 0 && space != (int)name.size()) {
        p->at = start + space;
        throw syntax_error(_errAt(p, "Found whitespace in your key name (use quotes to include)"));
      }
      return std::string(name.data(), (int)name.size());
    } else if (p->ch <= ' ') {
      if (p->ch == 0) {
        throw syntax_error(_errAt(p, "Found EOF while looking for a key name (check your syntax)"));
      }
      if (space < 0) {
        space = (int)name.size();
      }
    } else {
      if (_isPunctuatorChar(p->ch)) {
        throw syntax_error(_errAt(p, std::string("Found '") + (char)p->ch + std::string(
          "' where a key name was expected (check your syntax or use quotes if the key name includes {}[],: or whitespace)")));
      }
      name.push_back(p->ch);
    }
    _next(p);
  }
}


static void _white(Parser *p) {
  while (p->ch > 0) {
    // Skip whitespace.
    while (p->ch > 0 && p->ch <= ' ') {
      _next(p);
    }
    // Hjson allows comments
    if (p->ch == '#' || (p->ch == '/' && _peek(p, 0) == '/')) {
      while (p->ch > 0 && p->ch != '\n') {
        _next(p);
      }
    } else if (p->ch == '/' && _peek(p, 0) == '*') {
      _next(p);
      _next(p);
      while (p->ch > 0 && !(p->ch == '*' && _peek(p, 0) == '/')) {
        _next(p);
      }
      if (p->ch > 0) {
        _next(p);
        _next(p);
      }
    } else {
      break;
    }
  }
}


// Hjson strings can be quoteless
// returns string, true, false, or null.
static Value _readTfnns(Parser *p) {
  if (_isPunctuatorChar(p->ch)) {
    throw syntax_error(_errAt(p, std::string("Found a punctuator character '") +
      (char)p->ch + std::string("' when expecting a quoteless string (check your syntax)")));
  }
  auto chf = p->ch;
  std::vector<char> value;
  value.push_back(p->ch);

  for (;;) {
    _next(p);
    bool isEol = (p->ch == '\r' || p->ch == '\n' || p->ch == 0);
    if (isEol ||
      p->ch == ',' || p->ch == '}' || p->ch == ']' ||
      p->ch == '#' ||
      (p->ch == '/' && (_peek(p, 0) == '/' || _peek(p, 0) == '*')))
    {
      auto trimmed = _trim(std::string(value.data(), value.size()));

      switch (chf) {
      case 'f':
        if (trimmed == "false") {
          return false;
        }
        break;
      case 'n':
        if (trimmed == "null") {
          return Value(Value::HJSON_NULL);
        }
        break;
      case 't':
        if (trimmed == "true") {
          return true;
        }
        break;
      default:
        if (chf == '-' || (chf >= '0' && chf <= '9')) {
          double number;
          if (tryParseNumber(&number, value.data(), value.size(), false)) {
            return number;
          }
        }
      }
      if (isEol) {
        // remove any whitespace at the end (ignored in quoteless strings)
        return _trim(std::string(value.data(), value.size()));
      }
    }
    value.push_back(p->ch);
  }
}


// Parse an array value.
// assuming ch == '['
static Value _readArray(Parser *p) {
  Value array(Hjson::Value::VECTOR);

  _next(p);
  _white(p);

  if (p->ch == ']') {
    _next(p);
    return array; // empty array
  }

  while (p->ch > 0) {
    Value val = _readValue(p);
    array.push_back(val);
    _white(p);
    // in Hjson the comma is optional and trailing commas are allowed
    if (p->ch == ',') {
      _next(p);
      _white(p);
    }
    if (p->ch == ']') {
      _next(p);
      return array;
    }
    _white(p);
  }

  throw syntax_error(_errAt(p, "End of input while parsing an array (did you forget a closing ']'?)"));
}


// Parse an object value.
static Value _readObject(Parser *p, bool withoutBraces) {
  Value object(Hjson::Value::MAP);

  if (!withoutBraces) {
    // assuming ch == '{'
    _next(p);
  }

  _white(p);
  if (p->ch == '}' && !withoutBraces) {
    _next(p);
    return object; // empty object
  }
  while (p->ch > 0) {
    auto key = _readKeyname(p);
    _white(p);
    if (p->ch != ':') {
      throw syntax_error(_errAt(p, std::string(
        "Expected ':' instead of '") + (char)(p->ch) + "'"));
    }
    _next(p);
    // duplicate keys overwrite the previous value
    object[key] = _readValue(p);
    _white(p);
    // in Hjson the comma is optional and trailing commas are allowed
    if (p->ch == ',') {
      _next(p);
      _white(p);
    }
    if (p->ch == '}' && !withoutBraces) {
      _next(p);
      return object;
    }
    _white(p);
  }

  if (withoutBraces) {
    return object;
  }
  throw syntax_error(_errAt(p, "End of input while parsing an object (did you forget a closing '}'?)"));
}


// Parse a Hjson value. It could be an object, an array, a string, a number or a word.
static Value _readValue(Parser *p) {
  _white(p);

  switch (p->ch) {
  case '{':
    return _readObject(p, false);
  case '[':
    return _readArray(p);
  case '"':
  case '\'':
    return _readString(p, true);
  default:
    return _readTfnns(p);
  }
}


static Value _hasTrailing(Parser *p) {
  _white(p);
  return p->ch > 0;
}


// Braces for the root object are optional
static Value _rootValue(Parser *p) {
  Value res;

  _white(p);

  switch (p->ch) {
  case '{':
    res = _readObject(p, false);
    if (_hasTrailing(p)) {
      throw syntax_error(_errAt(p, "Syntax error, found trailing characters"));
    }
    return res;
  case '[':
    res = _readArray(p);
    if (_hasTrailing(p)) {
      throw syntax_error(_errAt(p, "Syntax error, found trailing characters"));
    }
    return res;
  }

  // assume we have a root object without braces
  try {
    res = _readObject(p, true);
    if (!_hasTrailing(p)) {
      return res;
    }
  } catch(syntax_error &e) {}

  // test if we are dealing with a single JSON value instead (true/false/null/num/"")
  _resetAt(p);
  res = _readValue(p);
  if (!_hasTrailing(p)) {
    return res;
  }

  throw syntax_error(_errAt(p, "Syntax error, found trailing characters"));
}


// Unmarshal parses the Hjson-encoded data and returns a tree of Values.
//
// Unmarshal uses the inverse of the encodings that Marshal uses.
//
Value Unmarshal(const char *data, size_t dataSize) {
  Parser parser = {
    (const unsigned char*) data,
    dataSize,
    0,
    ' '
  };

  _resetAt(&parser);
  return _rootValue(&parser);
}


Value Unmarshal(const char *data) {
  if (!data) {
    return Value();
  }

  return Unmarshal(data, std::strlen(data));
}


}
