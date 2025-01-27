/*******************************************************************\

Module: C++ Language Type Checking

Author: Daniel Kroening, kroening@cs.cmu.edu

\********************************************************************/

/// \file
/// C++ Language Type Checking

#include <util/c_types.h>
#include <util/symbol_table_base.h>

#include "cpp_declarator_converter.h"
#include "cpp_typecheck.h"
#include "cpp_util.h"

void cpp_typecheckt::convert(cpp_declarationt &declaration)
{
  // see if the declaration is empty
  if(declaration.is_empty())
    return;

  // The function bodies must not be checked here,
  // but only at the very end when all declarations have been
  // processed (or considering forward declarations at least)

  // templates are done in a dedicated function
  if(declaration.is_template())
    convert_template_declaration(declaration);
  else
    convert_non_template_declaration(declaration);
}

codet cpp_typecheckt::convert_anonymous_union(cpp_declarationt &declaration)
{
  codet new_code(ID_decl_block);
  new_code.reserve_operands(declaration.declarators().size());

  // unnamed object
  std::string identifier="#anon_union"+std::to_string(anon_counter++);

  const cpp_namet cpp_name(identifier, declaration.source_location());
  cpp_declaratort declarator;
  declarator.name()=cpp_name;

  cpp_declarator_convertert cpp_declarator_converter(*this);

  const symbolt &symbol=
    cpp_declarator_converter.convert(declaration, declarator);

  if(!cpp_is_pod(declaration.type()))
  {
    error().source_location=follow(declaration.type()).source_location();
    error() << "anonymous union is not POD" << eom;
    throw 0;
  }

  new_code.add_to_operands(code_frontend_declt(cpp_symbol_expr(symbol)));

  // do scoping
  symbolt &union_symbol =
    symbol_table.get_writeable_ref(follow(symbol.type).get(ID_name));

  for(const auto &c : to_union_type(union_symbol.type).components())
  {
    if(c.type().id() == ID_code)
    {
      error().source_location=union_symbol.type.source_location();
      error() << "anonymous union '" << union_symbol.base_name
              << "' shall not have function members" << eom;
      throw 0;
    }

    const irep_idt &base_name = c.get_base_name();

    if(cpp_scopes.current_scope().contains(base_name))
    {
      error().source_location=union_symbol.type.source_location();
      error() << "identifier '" << base_name << "' already in scope" << eom;
      throw 0;
    }

    cpp_idt &id=cpp_scopes.current_scope().insert(base_name);
    id.id_class = cpp_idt::id_classt::SYMBOL;
    id.identifier = c.get_name();
    id.class_identifier=union_symbol.name;
    id.is_member=true;
  }

  union_symbol.type.set(ID_C_unnamed_object, symbol.base_name);

  return new_code;
}

void cpp_typecheckt::convert_non_template_declaration(
  cpp_declarationt &declaration)
{
  PRECONDITION(!declaration.is_template());

  // we first check if this is a typedef
  typet &declaration_type=declaration.type();
  bool is_typedef=declaration.is_typedef();

  // the name anonymous tag types
  declaration.name_anon_struct_union();

  // do the type of the declaration
  if(declaration.declarators().empty() || !has_auto(declaration_type))
    typecheck_type(declaration_type);

  // Elaborate any class template instance _unless_ we do a typedef.
  // These are only elaborated on usage!
  if(!is_typedef)
    elaborate_class_template(declaration_type);

  // mark as 'already typechecked'
  if(!declaration.declarators().empty())
    already_typechecked_typet::make_already_typechecked(declaration_type);

  // Special treatment for anonymous unions
  if(declaration.declarators().empty() &&
     follow(declaration.type()).get_bool(ID_C_is_anonymous))
  {
    typet final_type=follow(declaration.type());

    if(final_type.id()!=ID_union)
    {
      error().source_location=final_type.source_location();
      error() << "top-level declaration does not declare anything"
              << eom;
      throw 0;
    }

    convert_anonymous_union(declaration);
  }

  // do the declarators (optional)
  for(auto &d : declaration.declarators())
  {
    // copy the declarator (we destroy the original)
    cpp_declaratort declarator=d;

    cpp_declarator_convertert cpp_declarator_converter(*this);

    cpp_declarator_converter.is_typedef=is_typedef;

    symbolt &symbol=cpp_declarator_converter.convert(
      declaration_type, declaration.storage_spec(),
      declaration.member_spec(), declarator);

    if(!symbol.is_type && !symbol.is_extern && symbol.type.id() == ID_empty)
    {
      error().source_location = symbol.location;
      error() << "void-typed symbol not permitted" << eom;
      throw 0;
    }

    // any template instance to remember?
    if(declaration.find(ID_C_template).is_not_nil())
    {
      symbol.type.set(ID_C_template, declaration.find(ID_C_template));
      symbol.type.set(
        ID_C_template_arguments,
        declaration.find(ID_C_template_arguments));
    }

    // replace declarator by symbol expression
    exprt tmp=cpp_symbol_expr(symbol);
    d.swap(tmp);

    // is there a constructor to be called for the declarator?
    if(symbol.is_lvalue &&
       declarator.init_args().has_operands())
    {
      auto constructor = cpp_constructor(
        symbol.location,
        cpp_symbol_expr(symbol),
        declarator.init_args().operands());

      if(constructor.has_value())
        symbol.value = constructor.value();
      else
        symbol.value = nil_exprt();
    }
  }
}
