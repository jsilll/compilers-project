#include <string>
#include "targets/type_checker.h"
#include ".auto/all_nodes.h"

#include "l22_parser.tab.h"

#define ASSERT_UNSPEC                                                 \
  {                                                                   \
    if (node->type() != nullptr && !node->is_typed(cdk::TYPE_UNSPEC)) \
      return;                                                         \
  }

//---------------------------------------------------------------------------

bool l22::type_checker::is_void_pointer(std::shared_ptr<cdk::reference_type> ptr)
{
  std::shared_ptr<cdk::basic_type> curr = ptr;
  while (curr->name() == cdk::TYPE_POINTER)
  {
    curr = cdk::reference_type::cast(curr)->referenced();
  }

  return curr->name() == cdk::TYPE_VOID;
}

std::shared_ptr<cdk::basic_type> l22::type_checker::same_pointer_types(std::shared_ptr<cdk::reference_type> leftPtr, std::shared_ptr<cdk::reference_type> rightPtr)
{
  std::shared_ptr<cdk::basic_type> left, right;
  left = leftPtr;
  right = rightPtr;

  while (left->name() == cdk::TYPE_POINTER && right->name() == cdk::TYPE_POINTER)
  {
    std::cout << left->name() << " " << right->name() << std::endl;
    left = cdk::reference_type::cast(left)->referenced();
    right = cdk::reference_type::cast(right)->referenced();
  }

  if (left->name() == cdk::TYPE_POINTER || right->name() == cdk::TYPE_POINTER)
  {
    std::cout << std::string("THROW Wrong pointer type.") << std::endl;
    throw std::string("Wrong pointer type.");
  }
  if (left->name() == cdk::TYPE_INT && right->name() == cdk::TYPE_INT)
  {
    return cdk::primitive_type::create(4, cdk::TYPE_INT);
  }
  else if (left->name() == cdk::TYPE_DOUBLE && right->name() == cdk::TYPE_DOUBLE)
  {
    return cdk::primitive_type::create(8, cdk::TYPE_DOUBLE);
  }
  else if (left->name() == cdk::TYPE_STRING && right->name() == cdk::TYPE_STRING)
  {
    return cdk::primitive_type::create(4, cdk::TYPE_STRING);
  }
  else
  {
    std::cout << std::string("THROW Wrong pointer type.") << std::endl;
    throw std::string("Wrong pointer type.");
  }
}

//---------------------------------------------------------------------------

void l22::type_checker::do_program_node(l22::program_node *const node, int lvl)
{
  std::cout << "void l22::type_checker::do_program_node(l22::program_node *const node, int lvl)" << std::endl;
  _lambda_stack.push(cdk::functional_type::create(cdk::primitive_type::create(4, cdk::TYPE_INT)));
}

//---------------------------------------------------------------------------

void l22::type_checker::do_data_node(cdk::data_node *const node, int lvl)
{
  std::cout << "void l22::type_checker::do_data_node(cdk::data_node *const node, int lvl)" << std::endl;
  // EMPTY
}

void l22::type_checker::do_sequence_node(cdk::sequence_node *const node, int lvl)
{
  std::cout << "void l22::type_checker::do_sequence_node(cdk::sequence_node *const node, int lvl)" << std::endl;
  for (size_t i = 0; i < node->size(); i++)
  {
    node->node(i)->accept(this, lvl + 2);
  }
}

void l22::type_checker::do_block_node(l22::block_node *node, int lvl)
{
  std::cout << "void l22::type_checker::do_block_node(l22::block_node *node, int lvl)" << std::endl;
  if (node->declarations())
  {
    node->declarations()->accept(this, lvl + 2);
  }
  if (node->instructions())
  {
    node->instructions()->accept(this, lvl + 2);
  }
}

void l22::type_checker::do_return_node(l22::return_node *node, int lvl)
{
  std::cout << "void l22::type_checker::do_return_node(l22::return_node *node, int lvl)" << std::endl;
  if (node->retval())
  {
    std::shared_ptr<cdk::functional_type> fType;
    if (!_lambda_stack.empty())
    {
      fType = _lambda_stack.top();
    }
    else
    {
      std::cout << std::string("THROW Return statement is not allowed outside of function.") << std::endl;
      throw std::string("Return statement is not allowed outside of function.");
    }

    if (fType->output(0)->name() == cdk::TYPE_VOID)
    {
      std::cout << std::string("THROW Void function cannot return values.") << std::endl;
      throw std::string("Void function cannot return values.");
    }

    node->retval()->accept(this, lvl + 2);

    if (fType->output(0)->name() == cdk::TYPE_INT)
    {
      if (!node->retval()->is_typed(cdk::TYPE_INT))
      {
        std::cout << std::string("THROW Wrong type for return statement (integer expected).") << std::endl;
        throw std::string("Wrong type for return statement (integer expected).");
      }
    }
    else if (fType->output(0)->name() == cdk::TYPE_DOUBLE)
    {
      if (!node->retval()->is_typed(cdk::TYPE_INT) && !node->retval()->is_typed(cdk::TYPE_DOUBLE))
      {
        std::cout << std::string("THROW Wrong type for return statement (integer or double expected).") << std::endl;
        throw std::string("Wrong type for return statement (integer or double expected).");
      }
    }
    else if (fType->output(0)->name() == cdk::TYPE_STRING)
    {
      if (!node->retval()->is_typed(cdk::TYPE_STRING))
      {
        std::cout << std::string("THROW Wrong type for return statement (string expected).") << std::endl;
        throw std::string("Wrong type for return statement (string expected).");
      }
    }
    else if (fType->output(0)->name() == cdk::TYPE_POINTER)
    {
      if (!node->retval()->is_typed(cdk::TYPE_POINTER))
      {
        std::cout << std::string("THROW Wrong type for return statement (pointer expected).") << std::endl;
        throw std::string("Wrong type for return statement (pointer expected).");
      }

      same_pointer_types(cdk::reference_type::cast(node->retval()->type()), cdk::reference_type::cast(fType->output(0)));
    }
    else if (fType->output(0)->name() == cdk::TYPE_FUNCTIONAL)
    {
      if (!node->retval()->is_typed(cdk::TYPE_FUNCTIONAL))
      {
        std::cout << std::string("THROW Wrong type for return statement (functional type expected).") << std::endl;
        throw std::string("Wrong type for return statement (functional type expected).");
      }

      std::shared_ptr<cdk::functional_type> fRetType = cdk::functional_type::cast(fType->output(0));
      std::shared_ptr<cdk::functional_type> retType = cdk::functional_type::cast(node->retval()->type());

      if (fRetType->output(0)->name() != retType->output(0)->name())
      {
        if (!(fRetType->output(0)->name() == cdk::TYPE_DOUBLE && retType->output(0)->name() == cdk::TYPE_INT))
        {
          std::cout << std::string("THROW Mismatching function output types in return statement.") << std::endl;
          throw std::string("Mismatching function output types in return statement.");
        }
      }

      if (fRetType->input_length() != retType->input_length())
      {
        std::cout << std::string("THROW Mismatching size of function arguments in return statement.") << std::endl;
        throw std::string("Mismatching size of function arguments in return statement.");
      }

      for (size_t i = 0; i < fRetType->input_length(); i++)
      {
        if (fRetType->input(i)->name() != retType->input(i)->name())
        {
          if (!(fRetType->input(0)->name() == cdk::TYPE_INT && retType->input(0)->name() == cdk::TYPE_DOUBLE))
          {
            std::cout << std::string("THROW Mismatching function argument types in return statement.") << std::endl;
            throw std::string("Mismatching function argument types in return statement.");
          }
        }
      }
    }
    else
    {
      std::cout << std::string("THROW Unknown type for return expression.") << std::endl;
      throw std::string("Unknown type for return expression.");
    }
  }
}

void l22::type_checker::do_declaration_node(l22::declaration_node *node, int lvl)
{
  std::cout << "void l22::type_checker::do_declaration_node(l22::declaration_node *node, int lvl)" << std::endl;
  if (node->type() == nullptr)
  {
    if (node->initializer() == nullptr)
    {
      std::cout << std::string("THROW Missing initialized in var declaration.") << std::endl;
      throw std::string("Missing initialized in var declaration.");
    }
    else
    {
      node->initializer()->accept(this, lvl + 2);
      if (node->initializer()->is_typed(cdk::TYPE_UNSPEC))
      {
        // l22::input_node case
        node->initializer()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
        node->type(node->initializer()->type());
      }
      else
      {
        node->type(node->initializer()->type());
      }
    }
  }
  else
  {
    if (node->initializer())
    {
      node->initializer()->accept(this, lvl + 2);
      if (node->initializer()->is_typed(cdk::TYPE_UNSPEC))
      {
        if (node->initializer()->is_typed(cdk::TYPE_UNSPEC))
        {
          // l22::input_node case
          if (node->is_typed(cdk::TYPE_INT) || node->is_typed(cdk::TYPE_DOUBLE))
          {
            node->initializer()->type(node->type());
          }
          else
          {
            std::cout << std::string("THROW Unable to read input.") << std::endl;
            throw std::string("Unable to read input.");
          }
        }
        else if (node->is_typed(cdk::TYPE_POINTER) && node->initializer()->is_typed(cdk::TYPE_POINTER))
        {
          // l22::stack_alloc_node case + nullptr case
          if (is_void_pointer(cdk::reference_type::cast(node->initializer()->type())))
          {
            node->initializer()->type(node->type());
          }
          else
          {
            same_pointer_types(cdk::reference_type::cast(node->type()), cdk::reference_type::cast(node->initializer()->type()));
            node->initializer()->type(node->type());
          }
        }
        else
        {
          std::cout << std::string("THROW Unknown node with unspecified type.") << std::endl;
          throw std::string("Unknown node with unspecified type.");
        }
      }
      else if (node->is_typed(cdk::TYPE_INT))
      {
        if (!node->initializer()->is_typed(cdk::TYPE_INT))
        {
          std::cout << std::string("THROW wrong type for initializer (integer expected).") << std::endl;
          throw std::string("wrong type for initializer (integer expected).");
        }
      }
      else if (node->is_typed(cdk::TYPE_DOUBLE))
      {
        if (!node->initializer()->is_typed(cdk::TYPE_INT) && !node->initializer()->is_typed(cdk::TYPE_DOUBLE))
        {
          std::cout << std::string("THROW wrong type for initializer (integer or double expected).") << std::endl;
          throw std::string("wrong type for initializer (integer or double expected).");
        }
      }
      else if (node->is_typed(cdk::TYPE_STRING))
      {
        if (!node->initializer()->is_typed(cdk::TYPE_STRING))
        {
          std::cout << std::string("THROW wrong type for initializer (string expected).") << std::endl;
          throw std::string("wrong type for initializer (string expected).");
        }
      }
      else if (node->is_typed(cdk::TYPE_POINTER))
      {
        if (!node->initializer()->is_typed(cdk::TYPE_POINTER))
        {
          std::cout << std::string("THROW Wrong type for initializer (pointer expected).") << std::endl;
          throw std::string("Wrong type for initializer (pointer expected).");
        }
      }
      else if (node->is_typed(cdk::TYPE_POINTER) && node->initializer()->is_typed(cdk::TYPE_POINTER))
      {
        // l22::nullptr_node and l22::stack_alloc_node
        if (is_void_pointer(cdk::reference_type::cast(node->initializer()->type())))
        {
          node->initializer()->type(node->type());
        }
        else
        {
          same_pointer_types(cdk::reference_type::cast(node->type()), cdk::reference_type::cast(node->initializer()->type()));
          node->initializer()->type(node->type());
        }
      }
      else if (node->is_typed(cdk::TYPE_FUNCTIONAL) && !node->initializer()->is_typed(cdk::TYPE_FUNCTIONAL))
      {
        std::cout << std::string("THROW Wrong type for initializer (lambda function expected).") << std::endl;
        throw std::string("Wrong type for initializer (lambda function expected).");
      }
      else if (node->is_typed(cdk::TYPE_FUNCTIONAL) && node->initializer()->is_typed(cdk::TYPE_FUNCTIONAL))
      {
        std::shared_ptr<cdk::functional_type> fType1 = cdk::functional_type::cast(node->type());
        std::shared_ptr<cdk::functional_type> fType2 = cdk::functional_type::cast(node->initializer()->type());
        if (fType1->output(0)->name() != fType2->output(0)->name())
        {
          if (!(fType1->output(0)->name() == cdk::TYPE_DOUBLE && fType2->output(0)->name() == cdk::TYPE_INT))
          {
            std::cout << std::string("THROW Mismatching function output types in declaration") << std::endl;
            throw std::string("Mismatching function output types in declaration");
          }
        }

        if (fType1->input_length() != fType2->input_length())
        {
          std::cout << std::string("THROW Mismatching size of function arguments in declaration.") << std::endl;
          throw std::string("Mismatching size of function arguments in declaration.");
        }

        for (size_t i = 0; i < fType1->input_length(); i++)
        {
          if (fType1->input(i)->name() != fType2->input(i)->name())
          {
            if (!(fType1->input(0)->name() == cdk::TYPE_INT && fType2->input(0)->name() == cdk::TYPE_DOUBLE))
            {
              std::cout << std::string("THROW Mismatching function argument types in declaration.") << std::endl;
              throw std::string("Mismatching function argument types in declaration.");
            }
          }
        }
      }
      else
      {
        std::cout << std::string("THROW Unkown type for initializer.") << std::endl;
        throw std::string("Unkown type for initializer.");
      }
    }
  }

  const std::string &id = node->identifier();
  auto symbol = l22::make_symbol(node->type(), id, node->qualifier(), node->qualifier(), (bool)node->initializer(), false);
  if (_symtab.insert(id, symbol))
  {
    _parent->set_new_symbol(symbol);
  }
  else if (!(_symtab.find(id)->qualifier() == tUSE))
  {
    std::cout << std::string("THROW variable '" + id + "' redeclared") << std::endl;
    throw std::string("variable '" + id + "' redeclared");
  }
}

//---------------------------------------------------------------------------

void l22::type_checker::do_nil_node(cdk::nil_node *const node, int lvl)
{
  std::cout << "void l22::type_checker::do_nil_node(cdk::nil_node *const node, int lvl)" << std::endl;
}

void l22::type_checker::do_again_node(l22::again_node *node, int lvl)
{
  std::cout << "void l22::type_checker::do_again_node(l22::again_node *node, int lvl)" << std::endl;
}

void l22::type_checker::do_stop_node(l22::stop_node *node, int lvl)
{
  std::cout << "void l22::type_checker::do_stop_node(l22::stop_node *node, int lvl)" << std::endl;
}

//---------------------------------------------------------------------------

void l22::type_checker::do_integer_node(cdk::integer_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_integer_node(cdk::integer_node *const node, int lvl)" << std::endl;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void l22::type_checker::do_double_node(cdk::double_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_double_node(cdk::double_node *const node, int lvl)" << std::endl;
  node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
}

void l22::type_checker::do_string_node(cdk::string_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_string_node(cdk::string_node *const node, int lvl)" << std::endl;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
}

void l22::type_checker::do_nullptr_node(l22::nullptr_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_nullptr_node(l22::nullptr_node *const node, int lvl)" << std::endl;
  node->type(cdk::reference_type::create(4, cdk::primitive_type::create(4, cdk::TYPE_VOID)));
}

//---------------------------------------------------------------------------

void l22::type_checker::do_variable_node(cdk::variable_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_variable_node(cdk::variable_node *const node, int lvl)" << std::endl;
  const std::string &id = node->name();
  std::shared_ptr<l22::symbol> symbol = _symtab.find(id);
  if (symbol)
  {
    node->type(symbol->type());
  }
  else if (id == "@")
  {
    if (!_lambda_stack.empty() && _lambda_stack.size() != 1)
    {
      node->type(_lambda_stack.top());
    }
    else
    {
      std::cout << std::string("Recursive call outside of function is not allowed.") << std::endl;
      throw std::string("Recursive call outside of function is not allowed.");
    }
  }
  else
  {
    std::cout << "THROW Undeclared variable '" + id + "'." << std::endl;
    throw "Undeclared variable '" + id + "'.";
  }
}

void l22::type_checker::do_index_node(l22::index_node *node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_index_node(l22::index_node *node, int lvl)" << std::endl;

  node->base()->accept(this, lvl + 2);
  if (!node->base()->is_typed(cdk::TYPE_POINTER))
  {
    std::cout << std::string("THROW Index left-value must be a pointer.") << std::endl;
    throw std::string("Index left-value must be a pointer.");
  }

  node->index()->accept(this, lvl + 2);
  if (node->index()->is_typed(cdk::TYPE_UNSPEC))
  {
    if (node->is_typed(cdk::TYPE_UNSPEC))
    {
      // l22::input_node case
      node->index()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
      std::cout << std::string("THROW Unknown node with unspecified type.") << std::endl;
      throw std::string("Unknown node with unspecified type.");
    }
  }
  else if (!node->index()->is_typed(cdk::TYPE_INT))
  {
    std::cout << std::string("THROW Integer expression expected in left-value index.") << std::endl;
    throw std::string("Integer expression expected in left-value index.");
  }

  node->type(cdk::reference_type::cast(node->base()->type())->referenced());
}

void l22::type_checker::do_rvalue_node(cdk::rvalue_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_rvalue_node(cdk::rvalue_node *const node, int lvl)" << std::endl;
  node->lvalue()->accept(this, lvl);
  node->type(node->lvalue()->type());
}

void l22::type_checker::do_assignment_node(cdk::assignment_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_assignment_node(cdk::assignment_node *const node, int lvl)" << std::endl;
  node->lvalue()->accept(this, lvl + 2);
  node->rvalue()->accept(this, lvl + 2);
  if (node->lvalue()->is_typed(cdk::TYPE_UNSPEC))
  {
    std::cout << std::string("THROW Left value must have a type.") << std::endl;
    throw std::string("Left value must have a type.");
  }

  // l22::input_node case
  if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC))
  {
    if (node->lvalue()->is_typed(cdk::TYPE_INT) || node->lvalue()->is_typed(cdk::TYPE_DOUBLE))
    {
      node->rvalue()->type(node->lvalue()->type());
    }
    else
    {
      std::cout << std::string("THROW Invalid expression for lvalue node.") << std::endl;
      throw std::string("Invalid expression for lvalue node.");
    }
  }
  else if (node->lvalue()->is_typed(cdk::TYPE_INT) && node->rvalue()->is_typed(cdk::TYPE_INT))
  {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if (node->lvalue()->is_typed(cdk::TYPE_DOUBLE) && (node->rvalue()->is_typed(cdk::TYPE_DOUBLE) || node->rvalue()->is_typed(cdk::TYPE_INT)))
  {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  }
  else if (node->lvalue()->is_typed(cdk::TYPE_STRING) && node->rvalue()->is_typed(cdk::TYPE_STRING))
  {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
  }
  else if (node->lvalue()->is_typed(cdk::TYPE_POINTER) && node->rvalue()->is_typed(cdk::TYPE_POINTER))
  {
    // l22::stack_alloc_node case + nullptr case
    if (is_void_pointer(cdk::reference_type::cast(node->rvalue()->type())))
    {
      node->type(node->lvalue()->type());
    }
    else
    {
      same_pointer_types(cdk::reference_type::cast(node->lvalue()->type()), cdk::reference_type::cast(node->rvalue()->type()));
      node->type(node->lvalue()->type());
    }
  }
  else if (node->lvalue()->is_typed(cdk::TYPE_FUNCTIONAL) && node->rvalue()->is_typed(cdk::TYPE_FUNCTIONAL))
  {
    std::shared_ptr<cdk::functional_type> fType1 = cdk::functional_type::cast(node->lvalue()->type());
    std::shared_ptr<cdk::functional_type> fType2 = cdk::functional_type::cast(node->rvalue()->type());
    if (fType1->output(0)->name() != fType2->output(0)->name())
    {
      if (!(fType1->output(0)->name() == cdk::TYPE_DOUBLE && fType2->output(0)->name() == cdk::TYPE_INT))
      {
        std::cout << std::string("THROW Mismatching function output types in assignment") << std::endl;
        throw std::string("Mismatching function output types in assignment");
      }
    }

    if (fType1->input_length() != fType2->input_length())
    {
      std::cout << std::string("THROW Mismatching size of function arguments in assignment.") << std::endl;
      throw std::string("Mismatching size of function arguments in assignment.");
    }

    for (size_t i = 0; i < fType1->input_length(); i++)
    {
      if (fType1->input(i)->name() != fType2->input(i)->name())
      {
        if (!(fType1->input(0)->name() == cdk::TYPE_INT && fType2->input(0)->name() == cdk::TYPE_DOUBLE))
        {
          std::cout << std::string("THROW Mismatching function argument types in assignment.") << std::endl;
          throw std::string("Mismatching function argument types in assignment.");
        }
      }
    }

    node->type(node->lvalue()->type());
  }
  else
  {
    std::cout << std::string("THROW Wrong types in assignment.") << std::endl;
    throw std::string("Wrong types in assignment.");
  }
}

//---------------------------------------------------------------------------

void l22::type_checker::do_UnaryExpression(cdk::unary_operation_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_UnaryExpression(cdk::unary_operation_node *const node, int lvl)" << std::endl;
  node->argument()->accept(this, lvl + 2);
  if (node->argument()->is_typed(cdk::TYPE_UNSPEC))
  {
    if (node->argument()->is_typed(cdk::TYPE_UNSPEC))
    {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
      node->argument()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
      std::cout << std::string("THROW Unknown node with unspecified type.") << std::endl;
      throw std::string("Unknown node with unspecified type.");
    }
  }
  else if (node->argument()->is_typed(cdk::TYPE_INT))
  {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if (node->argument()->is_typed(cdk::TYPE_DOUBLE))
  {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  }
  else
  {
    std::cout << std::string("THROW Wrong type in argument of unary expression (Integer or double expected).") << std::endl;
    throw std::string("Wrong type in argument of unary expression (Integer or double expected).");
  }
}

void l22::type_checker::do_neg_node(cdk::neg_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_neg_node(cdk::neg_node *const node, int lvl)" << std::endl;
  do_UnaryExpression(node, lvl);
}

void l22::type_checker::do_not_node(cdk::not_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_not_node(cdk::not_node *const node, int lvl)" << std::endl;
  node->argument()->accept(this, lvl + 2);
  if (node->argument()->is_typed(cdk::TYPE_INT))
  {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if (node->argument()->is_typed(cdk::TYPE_UNSPEC))
  {
    if (node->argument()->is_typed(cdk::TYPE_UNSPEC))
    {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
      node->argument()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
      std::cout << std::string("THROW Unknown type in argument of unary expression (Integer expected).") << std::endl;
      throw std::string("Unknown type in argument of unary expression (Integer expected).");
    }
  }
  else
  {
    std::cout << std::string("THROW Wrong type in argument of unary expression (Integer expected).") << std::endl;
    throw std::string("Wrong type in argument of unary expression (Integer expected).");
  }
}

void l22::type_checker::do_identity_node(l22::identity_node *node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_identity_node(l22::identity_node *node, int lvl)" << std::endl;
  do_UnaryExpression(node, lvl);
}

//---------------------------------------------------------------------------

void l22::type_checker::do_GeneralLogicalExpression(cdk::binary_operation_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_GeneralLogicalExpression(cdk::binary_operation_node *const node, int lvl)" << std::endl;

  node->left()->accept(this, lvl + 2);
  if (node->left()->is_typed(cdk::TYPE_UNSPEC))
  {
    node->left()->type(cdk::primitive_type::create(8, cdk::TYPE_INT));
  }

  node->right()->accept(this, lvl + 2);
  if (node->right()->is_typed(cdk::TYPE_UNSPEC))
  {
    node->left()->type(cdk::primitive_type::create(8, cdk::TYPE_INT));
  }

  if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_POINTER))
  {
    same_pointer_types(cdk::reference_type::cast(node->left()->type()), cdk::reference_type::cast(node->right()->type()));
  }
  else if (node->left()->type()->name() != node->right()->type()->name())
  {
    if (!((node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_DOUBLE)) || (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_INT))))
    {
      std::cout << std::string("THROW Operator has incompatible types.") << std::endl;
      throw std::string("Operator has incompatible types.");
    }
  }

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void l22::type_checker::do_BooleanLogicalExpression(cdk::binary_operation_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_BooleanLogicalExpression(cdk::binary_operation_node *const node, int lvl)" << std::endl;
  node->left()->accept(this, lvl + 2);

  if (node->left()->is_typed(cdk::TYPE_UNSPEC))
  {
    node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if (!node->left()->is_typed(cdk::TYPE_INT))
  {
    std::cout << std::string("THROW Integer expression expected in (left and right) binary operators.") << std::endl;
    throw std::string("Integer expression expected in (left and right) binary operators.");
  }

  node->right()->accept(this, lvl + 2);
  if (node->right()->is_typed(cdk::TYPE_UNSPEC))
  {
    node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if (!node->right()->is_typed(cdk::TYPE_INT))
  {
    std::cout << std::string("THROW Integer expression expected in (left and right) binary operators.") << std::endl;
    throw std::string("Integer expression expected in (left and right) binary operators.");
  }

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void l22::type_checker::do_IntOnlyExpression(cdk::binary_operation_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_IntOnlyExpression(cdk::binary_operation_node *const node, int lvl)" << std::endl;
  node->left()->accept(this, lvl + 2);
  if (node->left()->is_typed(cdk::TYPE_UNSPEC))
  {
    node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  node->right()->accept(this, lvl + 2);
  if (node->left()->is_typed(cdk::TYPE_UNSPEC))
  {
    node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_INT))
  {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else
  {
    std::cout << std::string("THROW Integer expression expected in (left and right) binary operators.") << std::endl;
    throw std::string("Integer expression expected in (left and right) binary operators.");
  }
}

void l22::type_checker::do_ScalarLogicalExpression(cdk::binary_operation_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_ScalarLogicalExpression(cdk::binary_operation_node *const node, int lvl)" << std::endl;

  node->left()->accept(this, lvl + 2);
  if (node->left()->is_typed(cdk::TYPE_UNSPEC))
  {
    node->left()->type(cdk::primitive_type::create(8, cdk::TYPE_INT));
  }
  else if (!node->left()->is_typed(cdk::TYPE_INT) && !node->left()->is_typed(cdk::TYPE_DOUBLE))
  {
    std::cout << std::string("THROW Wrong binary logical expression (expected integer or double).") << std::endl;
    throw std::string("Wrong binary logical expression (expected integer or double).");
  }

  node->right()->accept(this, lvl + 2);
  if (node->right()->is_typed(cdk::TYPE_UNSPEC))
  {
    node->right()->type(cdk::primitive_type::create(8, cdk::TYPE_INT));
  }
  else if (!node->right()->is_typed(cdk::TYPE_INT) && !node->right()->is_typed(cdk::TYPE_DOUBLE))
  {
    std::cout << std::string("THROW Wrong binary logical expression (expected integer or double).") << std::endl;
    throw std::string("Wrong binary logical expression (expected integer or double).");
  }

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void l22::type_checker::do_IDExpression(cdk::binary_operation_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_IDExpression(cdk::binary_operation_node *const node, int lvl)" << std::endl;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_DOUBLE))
  {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  }
  else if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT))
  {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  }
  else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_DOUBLE))
  {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  }
  else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_INT))
  {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if (node->left()->is_typed(cdk::TYPE_UNSPEC) && node->right()->is_typed(cdk::TYPE_UNSPEC))
  {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if (node->left()->is_typed(cdk::TYPE_UNSPEC) && (node->right()->is_typed(cdk::TYPE_INT) || node->right()->is_typed(cdk::TYPE_DOUBLE)))
  {
    node->left()->type(node->right()->type());
    node->type(node->right()->type());
  }
  else if (node->right()->is_typed(cdk::TYPE_UNSPEC) && (node->left()->is_typed(cdk::TYPE_INT) || node->left()->is_typed(cdk::TYPE_DOUBLE)))
  {
    node->right()->type(node->left()->type());
    node->type(node->left()->type());
  }
  else
  {
    std::cout << std::string("THROW Wrong types in binary expression.") << std::endl;
    throw std::string("Wrong types in binary expression.");
  }
}

//---------------------------------------------------------------------------

void l22::type_checker::do_add_node(cdk::add_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_add_node(cdk::add_node *const node, int lvl)" << std::endl;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);
  if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_DOUBLE))
  {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  }
  else if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT))
  {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  }
  else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_DOUBLE))
  {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  }
  else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_INT))
  {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_INT))
  {
    std::shared_ptr<cdk::reference_type> reference_type = cdk::reference_type::cast(node->left()->type());
    if (reference_type->referenced()->name() == cdk::TYPE_FUNCTIONAL)
    {
      std::cout << std::string("THROW Pointer arithmetic not supported for function pointers.") << std::endl;
      throw std::string("Pointer arithmetic not supported for function pointers.");
    }
    node->type(node->left()->type());
  }
  else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_POINTER))
  {
    std::shared_ptr<cdk::reference_type> reference_type = cdk::reference_type::cast(node->right()->type());
    if (reference_type->referenced()->name() == cdk::TYPE_FUNCTIONAL)
    {
      std::cout << std::string("THROW Pointer arithmetic not supported for function pointers.") << std::endl;
      throw std::string("Pointer arithmetic not supported for function pointers.");
    }
    node->type(node->right()->type());
  }
  else if (node->left()->is_typed(cdk::TYPE_UNSPEC) && node->right()->is_typed(cdk::TYPE_UNSPEC))
  {
    node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if (node->left()->is_typed(cdk::TYPE_UNSPEC))
  {
    if (node->right()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_INT))
    {
      node->left()->type(node->right()->type());
    }
    else
    {
      std::cout << std::string("THROW Invalid expression in right argument of binary expression.") << std::endl;
      throw std::string("Invalid expression in right argument of binary expression.");
    }
  }
  else if (node->right()->is_typed(cdk::TYPE_UNSPEC))
  {
    if (node->left()->is_typed(cdk::TYPE_DOUBLE) || node->left()->is_typed(cdk::TYPE_INT))
    {
      node->right()->type(node->left()->type());
    }
    else
    {
      std::cout << std::string("THROW Invalid expression in left argument of binary expression.") << std::endl;
      throw std::string("Invalid expression in left argument of binary expression.");
    }
  }
  else
  {
    std::cout << std::string("THROW Wrong types in binary expression.") << std::endl;
    throw std::string("Wrong types in binary expression.");
  }
}

void l22::type_checker::do_sub_node(cdk::sub_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_sub_node(cdk::sub_node *const node, int lvl)" << std::endl;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_DOUBLE))
  {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  }
  else if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT))
  {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  }
  else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_DOUBLE))
  {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  }
  else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_INT))
  {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_INT))
  {
    std::shared_ptr<cdk::reference_type> reference_type = cdk::reference_type::cast(node->left()->type());
    if (reference_type->referenced()->name() == cdk::TYPE_FUNCTIONAL)
    {
      std::cout << std::string("THROW Pointer arithmetic not supported for function pointers.") << std::endl;
      throw std::string("Pointer arithmetic not supported for function pointers.");
    }
    node->type(node->left()->type());
  }
  else if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_POINTER))
  {
    std::shared_ptr<cdk::reference_type> reference_type_left = cdk::reference_type::cast(node->left()->type());
    if (reference_type_left->referenced()->name() == cdk::TYPE_FUNCTIONAL)
    {
      std::cout << std::string("THROW Pointer arithmetic not supported for function pointers.") << std::endl;
      throw std::string("Pointer arithmetic not supported for function pointers.");
    }
    std::shared_ptr<cdk::reference_type> reference_type_right = cdk::reference_type::cast(node->right()->type());
    if (reference_type_right->referenced()->name() == cdk::TYPE_FUNCTIONAL)
    {
      std::cout << std::string("THROW Pointer arithmetic not supported for function pointers.") << std::endl;
      throw std::string("Pointer arithmetic not supported for function pointers.");
    }
    same_pointer_types(reference_type_left, reference_type_right);
    node->type(node->left()->type());
  }
  else if (node->left()->is_typed(cdk::TYPE_UNSPEC) && node->right()->is_typed(cdk::TYPE_UNSPEC))
  {
    node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if (node->left()->is_typed(cdk::TYPE_UNSPEC))
  {
    if (node->right()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_INT))
    {
      node->left()->type(node->right()->type());
    }
    else
    {
      std::cout << std::string("THROW Invalid expression in right argument of binary expression.") << std::endl;
      throw std::string("Invalid expression in right argument of binary expression.");
    }
  }
  else if (node->right()->is_typed(cdk::TYPE_UNSPEC))
  {
    if (node->left()->is_typed(cdk::TYPE_DOUBLE) || node->left()->is_typed(cdk::TYPE_INT))
    {
      node->right()->type(node->left()->type());
    }
    else
    {
      std::cout << std::string("THROW Invalid expression in left argument of binary expression.") << std::endl;
      throw std::string("Invalid expression in left argument of binary expression.");
    }
  }
  else
  {
    std::cout << std::string("THROW Wrong types in binary expression.") << std::endl;
    throw std::string("Wrong types in binary expression.");
  }
}

void l22::type_checker::do_mul_node(cdk::mul_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_mul_node(cdk::mul_node *const node, int lvl)" << std::endl;
  do_IDExpression(node, lvl);
}

void l22::type_checker::do_div_node(cdk::div_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_div_node(cdk::div_node *const node, int lvl)" << std::endl;
  do_IDExpression(node, lvl);
}

void l22::type_checker::do_mod_node(cdk::mod_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_mod_node(cdk::mod_node *const node, int lvl)" << std::endl;
  do_IntOnlyExpression(node, lvl);
}

//---------------------------------------------------------------------------

void l22::type_checker::do_gt_node(cdk::gt_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_gt_node(cdk::gt_node *const node, int lvl)" << std::endl;
  do_ScalarLogicalExpression(node, lvl);
}

void l22::type_checker::do_ge_node(cdk::ge_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_ge_node(cdk::ge_node *const node, int lvl)" << std::endl;
  do_ScalarLogicalExpression(node, lvl);
}

void l22::type_checker::do_le_node(cdk::le_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_le_node(cdk::le_node *const node, int lvl)" << std::endl;
  do_ScalarLogicalExpression(node, lvl);
}

void l22::type_checker::do_lt_node(cdk::lt_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_lt_node(cdk::lt_node *const node, int lvl)" << std::endl;
  do_ScalarLogicalExpression(node, lvl);
}

void l22::type_checker::do_eq_node(cdk::eq_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_eq_node(cdk::eq_node *const node, int lvl)" << std::endl;
  do_GeneralLogicalExpression(node, lvl);
}

void l22::type_checker::do_ne_node(cdk::ne_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_ne_node(cdk::ne_node *const node, int lvl)" << std::endl;
  do_GeneralLogicalExpression(node, lvl);
}

void l22::type_checker::do_and_node(cdk::and_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_and_node(cdk::and_node *const node, int lvl)" << std::endl;
  do_BooleanLogicalExpression(node, lvl);
}

void l22::type_checker::do_or_node(cdk::or_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_or_node(cdk::or_node *const node, int lvl)" << std::endl;
  do_BooleanLogicalExpression(node, lvl);
}

//---------------------------------------------------------------------------

void l22::type_checker::do_evaluation_node(l22::evaluation_node *const node, int lvl)
{
  std::cout << "void l22::type_checker::do_evaluation_node(l22::evaluation_node *const node, int lvl)" << std::endl;
  node->argument()->accept(this, lvl + 2);
  if (node->argument()->is_typed(cdk::TYPE_UNSPEC))
  {
    node->argument()->type(cdk::primitive_type::create(8, cdk::TYPE_INT));
  }
}

void l22::type_checker::do_print_node(l22::print_node *const node, int lvl)
{
  std::cout << "void l22::type_checker::do_print_node(l22::print_node *const node, int lvl)" << std::endl;
  node->arguments()->accept(this, lvl + 2);
}

void l22::type_checker::do_input_node(l22::input_node *node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_input_node(l22::input_node *node, int lvl)" << std::endl;
  node->type(cdk::primitive_type::create(0, cdk::TYPE_UNSPEC));
}

//---------------------------------------------------------------------------

void l22::type_checker::do_address_of_node(l22::address_of_node *node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_address_of_node(l22::address_of_node *node, int lvl)" << std::endl;
  node->lvalue()->accept(this, lvl + 2);
  node->type(cdk::reference_type::create(4, node->lvalue()->type()));
}

void l22::type_checker::do_stack_alloc_node(l22::stack_alloc_node *node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_stack_alloc_node(l22::stack_alloc_node *node, int lvl)" << std::endl;
  node->argument()->accept(this, lvl + 2);
  if (node->argument()->is_typed(cdk::TYPE_UNSPEC))
  {
    node->argument()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }

  if (!node->argument()->is_typed(cdk::TYPE_INT))
  {
    std::cout << std::string("THROW integer expression expected in allocation expression") << std::endl;
    throw std::string("integer expression expected in allocation expression");
  }

  node->type(cdk::reference_type::create(4, cdk::primitive_type::create(4, cdk::TYPE_VOID)));
}

//---------------------------------------------------------------------------

void l22::type_checker::do_while_node(l22::while_node *const node, int lvl)
{
  std::cout << "void l22::type_checker::do_while_node(l22::while_node *const node, int lvl)" << std::endl;
  node->condition()->accept(this, lvl + 2);
  node->condition()->accept(this, lvl + 2);
}

//---------------------------------------------------------------------------

void l22::type_checker::do_if_node(l22::if_node *const node, int lvl)
{
  std::cout << "void l22::type_checker::do_if_node(l22::if_node *const node, int lvl)" << std::endl;
  node->condition()->accept(this, lvl + 2);
  if (node->condition()->is_typed(cdk::TYPE_UNSPEC))
  {
    node->condition()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  if (!node->condition()->is_typed(cdk::TYPE_INT))
  {
    std::cout << std::string("THROW Expected integer condition") << std::endl;
    throw std::string("Expected integer condition");
  }
}

void l22::type_checker::do_if_else_node(l22::if_else_node *const node, int lvl)
{
  std::cout << "void l22::type_checker::do_if_else_node(l22::if_else_node *const node, int lvl)" << std::endl;
  node->condition()->accept(this, lvl + 2);
  if (node->condition()->is_typed(cdk::TYPE_UNSPEC))
  {
    node->condition()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  if (!node->condition()->is_typed(cdk::TYPE_INT))
  {
    std::cout << std::string("THROW Expected integer condition") << std::endl;
    throw std::string("Expected integer condition");
  }
}

//---------------------------------------------------------------------------

void l22::type_checker::do_function_call_node(l22::function_call_node *node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_function_call_node(l22::function_call_node *node, int lvl)" << std::endl;

  node->lambda_ptr()->accept(this, lvl + 2);
  if (!node->lambda_ptr()->is_typed(cdk::TYPE_FUNCTIONAL))
  {
    std::cout << std::string("THROW Left argument is not of functional type.") << std::endl;
    throw std::string("Left argument is not of functional type.");
  }

  std::shared_ptr<cdk::functional_type> fType = cdk::functional_type::cast(node->lambda_ptr()->type());

  if (node->arguments())
  {
    node->arguments()->accept(this, lvl + 2);
    if (fType->input_length() != node->arguments()->size())
    {
      std::cout << std::string("THROW Mismatching number of arguments in function call.") << std::endl;
      throw std::string("Mismatching number of arguments in function call.");
    }

    for (size_t i = 0; i < fType->input_length(); i++)
    {
      std::cout << node->argument(i)->type()->name() << " " << fType->input(i)->name() << std::endl;
      if (!(node->argument(i)->is_typed(fType->input(i)->name())))
      {
        if (!(node->argument(i)->is_typed(cdk::TYPE_INT) && fType->input(i)->name() == cdk::TYPE_DOUBLE))
        {
          std::cout << std::string("THROW Type mismatch for arguments in function call.") << std::endl;
          throw std::string("Type mismatch for arguments in function call.");
        }
      }
    }
  }
  else
  {
    if (fType->input_length() != 0)
    {
      std::cout << std::string("THROW Mismatching number of arguments in function call.") << std::endl;
      throw std::string("Mismatching number of arguments in function call.");
    }
  }

  node->type(fType->output()->component(0));
}

void l22::type_checker::do_lambda_node(l22::lambda_node *node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_lambda_node(l22::lambda_node *node, int lvl)" << std::endl;
  std::vector<std::shared_ptr<cdk::basic_type>> *argsTypes = new std::vector<std::shared_ptr<cdk::basic_type>>();
  if (node->arguments())
  {
    for (size_t i = 0; i < node->arguments()->size(); i++)
    {
      argsTypes->push_back(node->argument(i)->type());
    }
  }

  std::shared_ptr<cdk::functional_type> lambda_type = cdk::functional_type::create(*argsTypes, node->return_type());

  node->type(lambda_type);
  if (node->block())
  {
    _lambda_stack.push(lambda_type);
    _symtab.push();
    if (node->arguments())
    {
      node->arguments()->accept(this, lvl + 2);
    }
    node->block()->accept(this, lvl + 2);
    _symtab.pop();
    _lambda_stack.pop();
  }
}

//---------------------------------------------------------------------------

void l22::type_checker::do_sizeof_node(l22::sizeof_node *node, int lvl)
{
  ASSERT_UNSPEC;
  std::cout << "void l22::type_checker::do_sizeof_node(l22::sizeof_node *node, int lvl)" << std::endl;
  node->expression()->accept(this, lvl + 2);
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}
