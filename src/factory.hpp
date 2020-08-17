#pragma once

/*
 *  This file is part of Lime, a tool for the application of Likhtman & McLiesh' model for polymer dyanmics.
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 *
 *  File contents:
 * 
 *  Generic factory template
 *
 *  GPL 3.0 License
 * 
 */

#include <memory>
#include <map>
#include <functional>
#include <stdexcept>

//  USAGE
//
//  Register_class<Interface, Impl, Switch datatype, Arg datatypes...> some_unique_name(switch value);
//  typedef Factory_template<Interface, Switch datatype, Arg datatypes...> Factory;
//  Factory::Create(Switch value, Args..)
//

template< class Base, typename Key_type, class... Args >
class Factory_template
{
public:
   using Factory_function_type = std::function<std::unique_ptr<Base>(Args...)>;

   Factory_template(const Factory_template&) = delete;
   Factory_template &operator=(const Factory_template&) = delete;

   static void Register(const Key_type &key, Factory_function_type fn)
   {
      get_function_map()[key] = fn;
   }

   static std::unique_ptr<Base> create(const Key_type &key, Args... args)
   {
      auto iter = get_function_map().find(key);
      if (iter == get_function_map().end())
         throw std::runtime_error("Requested object with unknown key.");
      else
         return (iter->second)(args...);
   }

private:
   Factory_template() = default;

   typedef std::map<Key_type, Factory_function_type> Function_map;
  
   // Prevent static initialization fiasco by using getter
   static Function_map& get_function_map() {
      static Function_map function_list;
      return function_list;
   }
};

template <class Base, class Derived, typename Key_type, class... Args>
class Register_class
{
public:
   Register_class(const Key_type &key)
   {
      Factory_template<Base, Key_type, Args...>::Register(key, factory_function);
   }

   static std::unique_ptr<Base> factory_function(Args... args)
   {
      return std::make_unique<Derived>(args...);
   }
};