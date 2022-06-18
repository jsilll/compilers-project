#ifndef __L22_TARGETS_SYMBOL_H__
#define __L22_TARGETS_SYMBOL_H__

#include <string>
#include <memory>
#include <cdk/types/basic_type.h>

namespace l22
{

  class symbol
  {
    std::string _name;
    long _value;

    std::shared_ptr<cdk::basic_type> _type;
    bool _constant;
    int _qualifier; // public private etc
    bool _initialized;
    bool _function;

  public:
    symbol(std::shared_ptr<cdk::basic_type> type, const std::string &name, long value, bool constant, int qualifier, bool initialized)
        : _type(type), _name(name), _value(value), _constant(constant), _qualifier(qualifier), _initialized(initialized)
    {
    }

    virtual ~symbol()
    {
      // EMPTY
    }

    std::shared_ptr<cdk::basic_type> type() const
    {
      return _type;
    }
    bool is_typed(cdk::typename_type name) const
    {
      return _type->name() == name;
    }
    const std::string &name() const
    {
      return _name;
    }
    long value() const
    {
      return _value;
    }
    long value(long v)
    {
      return _value = v;
    }

    bool constant() const
    {
      return _constant;
    }
    int qualifier() const
    {
      return _qualifier;
    }

    std::shared_ptr<cdk::basic_type> type() const
    {
      return _type;
    }
    void set_type(std::shared_ptr<cdk::basic_type> t)
    {
      _type = t;
    }
    bool is_typed(cdk::typename_type name) const
    {
      return _type->name() == name;
    }

    const std::string &identifier() const
    {
      return name();
    }
    bool initialized() const
    {
      return _initialized;
    }

    bool isFunction() const
    {
      return _function;
    }

    bool isVariable() const
    {
      return !_function;
    }
  };

  inline auto make_symbol(bool constant, int qualifier, std::shared_ptr<cdk::basic_type> type, const std::string &name,
                          bool initialized)
  {
    return std::make_shared<symbol>(constant, qualifier, type, name, initialized);
  }

} // l22

#endif
