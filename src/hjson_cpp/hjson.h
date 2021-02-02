#ifndef HJSON_AFOWENFOWANEFWOAFNLL
#define HJSON_AFOWENFOWANEFWOAFNLL

#include <string>
#include <memory>
#include <map>
#include <stdexcept>


namespace Hjson {


class type_mismatch : public std::logic_error {
  using std::logic_error::logic_error;
};


class index_out_of_bounds : public std::out_of_range {
  using std::out_of_range::out_of_range;
};


class syntax_error : public std::runtime_error {
  using std::runtime_error::runtime_error;
};


// EncoderOptions defines options for encoding to Hjson.
struct EncoderOptions {
  // End of line, should be either \n or \r\n
  std::string eol;
  // Place braces on the same line
  bool bracesSameLine;
  // Always place string values in double quotation marks ("), and escape
  // any special chars inside the string value
  bool quoteAlways;
  // Always place keys in quotes
  bool quoteKeys;
  // Indent string
  std::string indentBy;
  // Allow the -0 value (unlike ES6)
  bool allowMinusZero;
  // Encode unknown values as 'null'
  bool unknownAsNull;
  // Output a comma separator between elements. If true, always place strings
  // in quotes (overriding the "quoteAlways" setting).
  bool separator;
};


class MapProxy;


class Value {
  friend class MapProxy;

private:
  class ValueImpl;
  std::shared_ptr<ValueImpl> prv;
  Value(std::shared_ptr<ValueImpl>);

public:
  enum Type {
    UNDEFINED,
    HJSON_NULL,
    BOOL,
    DOUBLE,
    STRING,
    VECTOR,
    MAP
  };

  Value();
  Value(bool);
  Value(double);
  Value(int);
  Value(const char*);
  Value(const std::string&);
  Value(Type);
  virtual ~Value();

  const Value operator[](const std::string&) const;
  MapProxy operator[](const std::string& name);
  const Value operator[](const char*) const;
  MapProxy operator[](const char*);
  const Value operator[](int) const;
  Value &operator[](int);
  bool operator ==(bool) const;
  bool operator !=(bool) const;
  bool operator ==(double) const;
  bool operator !=(double) const;
  bool operator ==(int) const;
  bool operator !=(int) const;
  bool operator ==(const char*) const;
  bool operator !=(const char*) const;
  bool operator ==(const std::string&) const;
  bool operator !=(const std::string&) const;
  bool operator ==(const Value&) const;
  bool operator !=(const Value&) const;
  bool operator <(double) const;
  bool operator >(double) const;
  bool operator <(int) const;
  bool operator >(int) const;
  bool operator <(const char*) const;
  bool operator >(const char*) const;
  bool operator <(const std::string&) const;
  bool operator >(const std::string&) const;
  bool operator <(const Value&) const;
  bool operator >(const Value&) const;
  double operator +(double) const;
  double operator +(int) const;
  std::string operator +(const char*) const;
  std::string operator +(const std::string&) const;
  Value operator +(const Value&) const;
  double operator -(double) const;
  double operator -(int) const;
  double operator -(const Value&) const;
  explicit operator bool() const;
  operator double() const;
  operator const char*() const;
  operator const std::string() const;

  bool defined() const;
  bool empty() const;
  Type type() const;
  size_t size() const;
  bool deep_equal(const Value&) const;
  Value clone() const;

  // VECTOR specific functions
  void erase(int);
  void push_back(const Value&);

  // MAP specific functions
  std::map<std::string, Value>::iterator begin();
  std::map<std::string, Value>::iterator end();
  std::map<std::string, Value>::const_iterator begin() const;
  std::map<std::string, Value>::const_iterator end() const;
  size_t erase(const std::string&);
  size_t erase(const char*);

  // Throws if used on VECTOR or MAP
  double to_double() const;
  std::string to_string() const;
};


class MapProxy : public Value {
  friend class Value;

private:
  std::shared_ptr<ValueImpl> parentPrv;
  std::string key;
  // True if an explicit assignment has been made to this MapProxy.
  bool wasAssigned;

  MapProxy(std::shared_ptr<ValueImpl> parent, std::shared_ptr<ValueImpl> child,
    const std::string &key);

public:
  ~MapProxy();
  MapProxy &operator =(const MapProxy&);
  MapProxy &operator =(const Value&);
};


EncoderOptions DefaultOptions();

// Returns a properly indented text representation of the input value tree.
// Extra options can be specified in the input parameter "options".
std::string MarshalWithOptions(Value v, EncoderOptions options);

// Returns a properly indented text representation of the input value tree.
std::string Marshal(Value v);

// Returns a properly indented JSON text representation of the input value
// tree.
std::string MarshalJson(Value v);

// Creates a Value tree from input text.
Value Unmarshal(const char *data, size_t dataSize);

// Creates a Value tree from input text.
// The input parameter "data" must be null-terminated.
Value Unmarshal(const char *data);

// Returns a Value tree that is a combination of the input parameters "base"
// and "ext".
//
// If "base" and "ext" contain a map on the same place in the tree, the
// returned tree will on that place have a map containing a combination of
// all keys from the "base" and "ext" maps. If a key existed in both "base"
// and "ext", the value from "ext" is used. Except for if the value in "ext"
// is of type UNDEFINED: then the value from "base" is used.
//
// Vectors are not merged: if a vector exists in the same place in the "base"
// and "ext" trees, the one from "ext" will be used in the returned tree.
//
// Maps and vectors are cloned, not copied. Therefore changes in the returned
// tree will not affect the input variables "base" and "ext.
//
// If "ext" is of type UNDEFINED, a clone of "base" is returned.
//
Value Merge(const Value base, const Value ext);


}


#endif
