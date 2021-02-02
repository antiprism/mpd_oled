#include "hjson.h"
#include <sstream>
#include <vector>
#include <assert.h>
#include <cstring>


namespace Hjson {


typedef std::vector<Value> ValueVec;
typedef std::map<std::string, Value> ValueMap;


class Value::ValueImpl {
public:
  Type type;
  union {
    bool b;
    double d;
    void *p;
  };

  ValueImpl();
  ValueImpl(bool);
  ValueImpl(double);
  ValueImpl(const std::string&);
  ValueImpl(Type);
  ~ValueImpl();
};


Value::ValueImpl::ValueImpl()
  : type(UNDEFINED)
{
}


Value::ValueImpl::ValueImpl(bool input)
  : type(BOOL),
  b(input)
{
}


Value::ValueImpl::ValueImpl(double input)
  : type(DOUBLE),
  d(input)
{
}


Value::ValueImpl::ValueImpl(const std::string &input)
  : type(STRING),
  p(new std::string(input))
{
}


Value::ValueImpl::ValueImpl(Type _type)
  : type(_type)
{
  switch (_type)
  {
  case DOUBLE:
    d = 0.0;
    break;
  case STRING:
    p = new std::string();
    break;
  case VECTOR:
    p = new ValueVec();
    break;
  case MAP:
    p = new ValueMap();
    break;
  default:
    break;
  }
}


Value::ValueImpl::~ValueImpl() {
  switch (type)
  {
  case STRING:
    delete (std::string*) p;
    break;
  case VECTOR:
    delete (ValueVec*)p;
    break;
  case MAP:
    delete (ValueMap*)p;
    break;
  default:
    break;
  }
}


Value::Value(std::shared_ptr<ValueImpl> _prv)
  : prv(_prv)
{
}


// Sacrifice efficiency for predictability: It is allowed to do bracket
// assignment on an UNDEFINED Value, and thereby turn it into a MAP Value.
// A MAP Value is passed by reference, therefore an UNDEFINED Value should also
// be passed by reference, to avoid surprises when doing bracket assignment
// on a Value that has been passed around but is still of type UNDEFINED.
Value::Value()
  : prv(std::make_shared<ValueImpl>(UNDEFINED))
{
}


Value::Value(bool input)
  : prv(std::make_shared<ValueImpl>(input))
{
}


Value::Value(double input)
  : prv(std::make_shared<ValueImpl>(input))
{
}


Value::Value(int input)
  : prv(std::make_shared<ValueImpl>((double) input))
{
}


Value::Value(const char *input)
  : prv(std::make_shared<ValueImpl>(std::string(input)))
{
}


Value::Value(const std::string& input)
  : prv(std::make_shared<ValueImpl>(input))
{
}


Value::Value(Type _type)
  : prv(std::make_shared<ValueImpl>(_type))
{
}


Value::~Value() {
}


const Value Value::operator[](const std::string& name) const {
  if (prv->type == UNDEFINED) {
    return Value();
  } else if (prv->type == MAP) {
    auto it = ((ValueMap*)prv->p)->find(name);
    if (it == ((ValueMap*)prv->p)->end()) {
      return Value();
    }
    return it->second;
  }

  throw type_mismatch("Must be of type UNDEFINED or MAP for that operation.");
}


MapProxy Value::operator[](const std::string& name) {
  if (prv->type == UNDEFINED) {
    prv->~ValueImpl();
    // Recreate the private object using the same memory block.
    new(&(*prv)) ValueImpl(MAP);
  } else if (prv->type != MAP) {
    throw type_mismatch("Must be of type UNDEFINED or MAP for that operation.");
  }

  auto it = ((ValueMap*)prv->p)->find(name);
  if (it == ((ValueMap*)prv->p)->end()) {
    return MapProxy(prv, std::make_shared<ValueImpl>(UNDEFINED), name);
  }
  return MapProxy(prv, it->second.prv, name);
}


const Value Value::operator[](const char *input) const {
  return operator[](std::string(input));
}


MapProxy Value::operator[](const char *input) {
  return operator[](std::string(input));
}


const Value Value::operator[](int index) const {
  if (prv->type == UNDEFINED) {
    throw index_out_of_bounds("Index out of bounds.");
  } else if (prv->type != VECTOR) {
    throw type_mismatch("Must be of type UNDEFINED or VECTOR for that operation.");
  }

  if (index < 0 || index >= (int)size()) {
    throw index_out_of_bounds("Index out of bounds.");
  }

  return ((const ValueVec*)prv->p)[0][index];
}


Value &Value::operator[](int index) {
  if (prv->type == UNDEFINED) {
    throw index_out_of_bounds("Index out of bounds.");
  } else if (prv->type != VECTOR) {
    throw type_mismatch("Must be of type UNDEFINED or VECTOR for that operation.");
  }

  if (index < 0 || index >= (int)size()) {
    throw index_out_of_bounds("Index out of bounds.");
  }

  return ((ValueVec*)prv->p)[0][index];
}


bool Value::operator==(bool input) const {
  return operator bool() == input;
}


bool Value::operator!=(bool input) const {
  return !(*this == input);
}


bool Value::operator==(double input) const {
  return operator double() == input;
}


bool Value::operator!=(double input) const {
  return !(*this == input);
}


bool Value::operator==(int input) const {
  return operator double() == input;
}


bool Value::operator!=(int input) const {
  return !(*this == input);
}


bool Value::operator==(const char *input) const {
  return !strcmp(operator const char*(), input);
}


bool Value::operator!=(const char *input) const {
  return !(*this == input);
}


bool Value::operator==(const std::string &input) const {
  return operator const std::string() == input;
}


bool Value::operator!=(const std::string &input) const {
  return !(*this == input);
}


bool Value::operator==(const Value &other) const {
  if (prv->type != other.prv->type) {
    return false;
  }

  switch (prv->type) {
  case UNDEFINED:
  case HJSON_NULL:
    return true;
  case BOOL:
    return prv->b == other.prv->b;
  case DOUBLE:
    return prv->d == other.prv->d;
  case STRING:
    return *((std::string*) prv->p) == *((std::string*)other.prv->p);
  case VECTOR:
  case MAP:
    return prv->p == other.prv->p;
  }

  assert(!"Unknown type");

  return false;
}


bool Value::operator!=(const Value &other) const {
  return !(*this == other);
}


bool Value::operator>(double input) const {
  return operator double() > input;
}


bool Value::operator<(double input) const {
  return operator double() < input;
}


bool Value::operator>(int input) const {
  return operator double() > input;
}


bool Value::operator<(int input) const {
  return operator double() < input;
}


bool Value::operator>(const char *input) const {
  return operator const std::string() > input;
}


bool Value::operator<(const char *input) const {
  return operator const std::string() < input;
}


bool Value::operator>(const std::string &input) const {
  return operator const std::string() > input;
}


bool Value::operator<(const std::string &input) const {
  return operator const std::string() < input;
}


bool Value::operator>(const Value &other) const {
  if (prv->type != other.prv->type) {
    throw type_mismatch("The compared values must be of the same type.");
  }

  switch (prv->type) {
  case DOUBLE:
    return prv->d > other.prv->d;
  case STRING:
    return *((std::string*)prv->p) > *((std::string*)other.prv->p);
  default:
    throw type_mismatch("The compared values must be of type DOUBLE or STRING.");
  }
}


bool Value::operator<(const Value &other) const {
  if (prv->type != other.prv->type) {
    throw type_mismatch("The compared values must be of the same type.");
  }

  switch (prv->type) {
  case DOUBLE:
    return prv->d < other.prv->d;
  case STRING:
    return *((std::string*)prv->p) < *((std::string*)other.prv->p);
  default:
    throw type_mismatch("The compared values must be of type DOUBLE or STRING.");
  }
}


double Value::operator+(int input) const {
  return operator double() + input;
}


double Value::operator+(double input) const {
  return operator double() + input;
}


std::string Value::operator+(const char *input) const {
  return operator const std::string() + input;
}


std::string Value::operator+(const std::string &input) const {
  return operator const std::string() + input;
}


Value Value::operator+(const Value &other) const {
  if (prv->type != other.prv->type) {
    throw type_mismatch("The values must be of the same type for this operation.");
  }

  switch (prv->type) {
  case DOUBLE:
    return prv->d + other.prv->d;
  case STRING:
    return *((std::string*)prv->p) + *((std::string*)other.prv->p);
  default:
    throw type_mismatch("The values must be of type DOUBLE or STRING for this operation.");
  }
}


double Value::operator-(int input) const {
  return operator double() - input;
}


double Value::operator-(double input) const {
  return operator double() - input;
}


double Value::operator-(const Value &other) const {
  return operator double() - other.operator double();
}


Value::operator bool() const {
  return (prv->type == DOUBLE ? !!prv->d : (prv->type == BOOL ? prv->b : !empty()));
}


Value::operator double() const {
  if (prv->type != DOUBLE) {
    throw type_mismatch("Must be of type DOUBLE for that operation.");
  }

  return prv->d;
}


Value::operator const char*() const {
  if (prv->type != STRING) {
    throw type_mismatch("Must be of type STRING for that operation.");
  }

  return ((std::string*)(prv->p))->c_str();
}


Value::operator const std::string() const {
  if (prv->type != STRING) {
    throw type_mismatch("Must be of type STRING for that operation.");
  }

  return *((const std::string*)(prv->p));
}


bool Value::defined() const {
  return prv->type != UNDEFINED;
}


bool Value::empty() const {
  return (prv->type == UNDEFINED ||
    prv->type == HJSON_NULL ||
    (prv->type == STRING && ((std::string*)prv->p)->empty()) ||
    (prv->type == VECTOR && ((ValueVec*)prv->p)->empty()) ||
    (prv->type == MAP && ((ValueMap*)prv->p)->empty()));
}


Value::Type Value::type() const {
  return prv->type;
}


size_t Value::size() const {
  if (prv->type == UNDEFINED || prv->type == HJSON_NULL) {
    return 0;
  }

  switch (prv->type)
  {
  case STRING:
    return ((std::string*)prv->p)->size();
  case VECTOR:
    return ((ValueVec*)prv->p)->size();
  case MAP:
    return ((ValueMap*)prv->p)->size();
  default:
    break;
  }

  assert(prv->type == BOOL || prv->type == DOUBLE);

  return 1;
}


bool Value::deep_equal(const Value &other) const {
  if (*this == other) {
    return true;
  }

  if (this->type() != other.type() || this->size() != other.size()) {
    return false;
  }

  switch (prv->type)
  {
  case VECTOR:
    {
      auto itA = ((ValueVec*)(this->prv->p))->begin();
      auto endA = ((ValueVec*)(this->prv->p))->end();
      auto itB = ((ValueVec*)(other.prv->p))->begin();
      while (itA != endA) {
        if (!itA->deep_equal(*itB)) {
          return false;
        }
        ++itA;
        ++itB;
      }
    }
    return true;

  case MAP:
    {
      auto itA = this->begin(), endA = this->end(), itB = other.begin();
      while (itA != endA) {
        if (!itA->second.deep_equal(itB->second)) {
          return false;
        }
        ++itA;
        ++itB;
      }
    }
    return true;

  default:
    return false;
  }
}


Value Value::clone() const {
  switch (type()) {
  case VECTOR:
    {
      Value ret;
      for (int index = 0; index < int(size()); ++index) {
        ret.push_back(operator[](index).clone());
      }
      return ret;
    }

  case MAP:
    {
      Value ret;
      for (auto it = begin(); it != end(); ++it) {
        ret[it->first] = it->second.clone();
      }
      return ret;
    }
  default:
    return *this;
  }
}


void Value::erase(int index) {
  if (prv->type != UNDEFINED && prv->type != VECTOR) {
    throw type_mismatch("Must be of type VECTOR for that operation.");
  } else if (index < 0 || index >= (int)size()) {
    throw index_out_of_bounds("Index out of bounds.");
  }

  auto vec = (ValueVec*) prv->p;
  vec->erase(vec->begin() + index);
}


void Value::push_back(const Value &other) {
  if (prv->type == UNDEFINED) {
    prv->~ValueImpl();
    // Recreate the private object using the same memory block.
    new(&(*prv)) ValueImpl(VECTOR);
  } else if (prv->type != VECTOR) {
    throw type_mismatch("Must be of type UNDEFINED or VECTOR for that operation.");
  }

  ((ValueVec*)prv->p)->push_back(other);
}


ValueMap::iterator Value::begin() {
  if (prv->type != MAP) {
    // Some C++ compilers might not allow comparing this to another
    // default-constructed iterator.
    return ValueMap::iterator();
  }

  return ((ValueMap*)prv->p)->begin();
}


ValueMap::iterator Value::end() {
  if (prv->type != MAP) {
    // Some C++ compilers might not allow comparing this to another
    // default-constructed iterator.
    return ValueMap::iterator();
  }

  return ((ValueMap*)prv->p)->end();
}


ValueMap::const_iterator Value::begin() const {
  if (prv->type != MAP) {
    // Some C++ compilers might not allow comparing this to another
    // default-constructed iterator.
    return ValueMap::const_iterator();
  }

  return ((const ValueMap*)prv->p)->begin();
}


ValueMap::const_iterator Value::end() const {
  if (prv->type != MAP) {
    // Some C++ compilers might not allow comparing this to another
    // default-constructed iterator.
    return ValueMap::const_iterator();
  }

  return ((const ValueMap*)prv->p)->end();
}


size_t Value::erase(const std::string &key) {
  if (prv->type == UNDEFINED) {
    return 0;
  } else if (prv->type != MAP) {
    throw type_mismatch("Must be of type MAP for that operation.");
  }

  return ((ValueMap*)(prv->p))->erase(key);
}


size_t Value::erase(const char *key) {
  return erase(std::string(key));
}


double Value::to_double() const {
  switch (prv->type) {
  case UNDEFINED:
  case HJSON_NULL:
    return 0;
  case BOOL:
    return (prv->b ? 1 : 0);
  case DOUBLE:
    return prv->d;
  case STRING: {
    double ret;
    std::stringstream ss(*((std::string*)prv->p));

    // Make sure we expect dot (not comma) as decimal point.
    ss.imbue(std::locale::classic());

    ss >> ret;

    if (!ss.eof() || ss.fail()) {
      return 0.0;
    }

    return ret;
  }
  default:
    throw type_mismatch("Illegal type for this operation.");
  }

}


std::string Value::to_string() const {
  switch (prv->type) {
  case UNDEFINED:
    return "";
  case HJSON_NULL:
    return "null";
  case BOOL:
    return (prv->b ? "true" : "false");
  case DOUBLE: {
    std::ostringstream oss;

    // Make sure we expect dot (not comma) as decimal point.
    oss.imbue(std::locale::classic());
    oss.precision(15);

    oss << prv->d;

    return oss.str();
  }
  case STRING:
    return *((std::string*)prv->p);
  default:
    throw type_mismatch("Illegal type for this operation.");
  }
}


MapProxy::MapProxy(std::shared_ptr<ValueImpl> _parent,
  std::shared_ptr<ValueImpl> _child, const std::string &_key)
  : parentPrv(_parent),
    key(_key),
    wasAssigned(false)
{
  prv = _child;
}


MapProxy::~MapProxy() {
  if (wasAssigned || !empty()) {
    // We waited until now because we don't want to insert a Value object of
    // type UNDEFINED into the parent map, unless such an object was explicitly
    // assigned (e.g. `val["key"] = Hjson::Value()`).
    // Without this requirement, checking for the existence of an element
    // would create an UNDEFINED element for that key if it didn't already exist
    // (e.g. `if (val["key"] == 1) {` would create an element for "key").
    ((ValueMap*)parentPrv->p)[0][key] = Value(prv);
  }
}


MapProxy &MapProxy::operator =(const MapProxy &other) {
  prv = other.prv;
  wasAssigned = true;
  return *this;
}


MapProxy &MapProxy::operator =(const Value &other) {
  prv = other.prv;
  wasAssigned = true;
  return *this;
}


Value Merge(const Value base, const Value ext) {
  Value merged;

  if (!ext.defined()) {
    merged = base.clone();
  } else if (base.type() == Value::MAP && ext.type() == Value::MAP) {
    for (auto it = ext.begin(); it != ext.end(); ++it) {
      merged[it->first] = Merge(base[it->first], it->second);
    }

    for (auto it = base.begin(); it != base.end(); ++it) {
      if (!merged[it->first].defined()) {
        merged[it->first] = it->second.clone();
      }
    }
  } else {
    merged = ext.clone();
  }

  return merged;
}


}
