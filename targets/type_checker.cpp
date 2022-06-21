#include <string>
#include "targets/type_checker.h"
#include ".auto/all_nodes.h"

#define ASSERT_UNSPEC                                                 \
  {                                                                   \
    if (node->type() != nullptr && !node->is_typed(cdk::TYPE_UNSPEC)) \
      return;                                                         \
  }

//---------------------------------------------------------------------------

std::shared_ptr<cdk::basic_type> l22::type_checker::typeOfPointer(std::shared_ptr<cdk::reference_type> leftPtr, std::shared_ptr<cdk::reference_type> rightPtr)
{
  std::shared_ptr<cdk::basic_type> left, right;
  left = leftPtr;
  right = rightPtr;
  while (left->name() == cdk::TYPE_POINTER && right->name() == cdk::TYPE_POINTER)
  {
    left = cdk::reference_type::cast(left)->referenced();
    right = cdk::reference_type::cast(right)->referenced();
  }
  if (left->name() == cdk::TYPE_POINTER || right->name() == cdk::TYPE_POINTER)
  {
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
    throw std::string("Wrong pointer type.");
  }
}

//---------------------------------------------------------------------------

void l22::type_checker::do_program_node(l22::program_node *const node, int lvl)
{
  // colocar aqui a criacao do simbolo e maybe ver em dar push para a lambda stack
  auto lambda = cdk::functional_type::create(cdk::primitive_type::create(4, cdk::TYPE_INT));
  _parent->set_new_lambda(lambda);
}

//---------------------------------------------------------------------------

void l22::type_checker::do_data_node(cdk::data_node *const node, int lvl)
{
  // EMPTY
}

void l22::type_checker::do_sequence_node(cdk::sequence_node *const node, int lvl)
{
  for (size_t i = 0; i < node->size(); i++)
  {
    node->node(i)->accept(this, lvl + 2);
    cdk::expression_node *expression = dynamic_cast<cdk::expression_node *>(node->node(i));
    if (expression != nullptr && expression->is_typed(cdk::TYPE_UNSPEC))
    {
      l22::input_node *input = dynamic_cast<l22::input_node *>(expression);
      if (input != nullptr)
      {
        input->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
      }
      else
      {
        throw std::string("Unknown node with unspecified type.");
      }
    }
  }
}

void l22::type_checker::do_block_node(l22::block_node *node, int lvl)
{
  // EMPTY
}

void l22::type_checker::do_return_node(l22::return_node *node, int lvl)
{
  if (node->retval())
  {
    std::shared_ptr<cdk::functional_type> fType;
    if (_lambda_stack.size())
    {
      fType = _lambda_stack.top();
    }
    else
    {
      throw std::string("Return statement is not allowed outside of function.");
    }

    if (fType->output()->name() == cdk::TYPE_VOID)
    {
      throw std::string("Void function cannot return values.");
    }

    node->retval()->accept(this, lvl + 2);

    if (fType->output()->component(0)->name() == cdk::TYPE_INT)
    {
      if (!node->retval()->is_typed(cdk::TYPE_INT))
      {
        throw std::string("Wrong type for initializer (integer expected).");
      }
    }
    else if (fType->output()->component(0)->name() == cdk::TYPE_DOUBLE)
    {
      if (!node->retval()->is_typed(cdk::TYPE_INT) && !node->retval()->is_typed(cdk::TYPE_DOUBLE))
      {
        throw std::string("Wrong type for initializer (integer or double expected).");
      }
    }
    else if (fType->output()->component(0)->name() == cdk::TYPE_STRING)
    {
      if (!node->retval()->is_typed(cdk::TYPE_STRING))
      {
        throw std::string("Wrong type for initializer (string expected).");
      }
    }
    else if (fType->output()->component(0)->name() == cdk::TYPE_POINTER)
    {
      typeOfPointer(cdk::reference_type::cast(node->retval()->type()), cdk::reference_type::cast(fType->output()));
    }
    else
    {
      throw std::string("Unknown type for return expression.");
    }
  }
}

void l22::type_checker::do_declaration_node(l22::declaration_node *node, int lvl)
{
  if (node->type() == nullptr)
  {
    if (node->initializer() == nullptr)
    {
      throw std::string("Missing initialized in var declaration.");
    }
    else
    {
      node->initializer()->accept(this, lvl + 2);
      if (node->initializer()->is_typed(cdk::TYPE_UNSPEC))
      {
        l22::input_node *input = dynamic_cast<l22::input_node *>(node->initializer());
        if (input != nullptr)
        {
          node->initializer()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
          node->type(node->initializer()->type());
        }
        else
        {
          throw std::string("Unknown node with unspecified type.");
        }
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
        l22::input_node *input = dynamic_cast<l22::input_node *>(node->initializer());
        l22::stack_alloc_node *stack = dynamic_cast<l22::stack_alloc_node *>(node->initializer());
        if (input != nullptr)
        {
          if (node->is_typed(cdk::TYPE_INT) || node->is_typed(cdk::TYPE_DOUBLE))
          {
            node->initializer()->type(node->type());
          }
          else
          {
            throw std::string("Unable to read input.");
          }
        }
        else if (stack != nullptr)
        {
          if (node->is_typed(cdk::TYPE_POINTER))
          {
            node->initializer()->type(node->type());
          }
        }
        else
        {
          throw std::string("Unknown node with unspecified type.");
        }
      }
      else if (node->is_typed(cdk::TYPE_INT))
      {
        if (!node->initializer()->is_typed(cdk::TYPE_INT))
          throw std::string("wrong type for initializer (integer expected).");
      }
      else if (node->is_typed(cdk::TYPE_DOUBLE) && !node->initializer()->is_typed(cdk::TYPE_INT) && !node->initializer()->is_typed(cdk::TYPE_DOUBLE))
      {
        throw std::string("wrong type for initializer (integer or double expected).");
      }
      else if (node->is_typed(cdk::TYPE_STRING) && !node->initializer()->is_typed(cdk::TYPE_STRING))
      {
        throw std::string("wrong type for initializer (string expected).");
      }
      else if (node->is_typed(cdk::TYPE_POINTER) && !node->initializer()->is_typed(cdk::TYPE_POINTER))
      {
        throw std::string("Wrong type for initializer (pointer expected).");
      }
      else if (node->is_typed(cdk::TYPE_POINTER) && node->initializer()->is_typed(cdk::TYPE_POINTER))
      {
        // TODO:
        l22::nullptr_node *n = dynamic_cast<l22::nullptr_node *>(node->initializer());
        if (n == nullptr)
        {
          typeOfPointer(cdk::reference_type::cast(node->type()), cdk::reference_type::cast(node->initializer()->type()));
        }
      }
      else if (node->is_typed(cdk::TYPE_FUNCTIONAL) && !node->initializer()->is_typed(cdk::TYPE_FUNCTIONAL))
      {
        //  TODO: condição tem que ser mais forte do que isto
        throw std::string("Wrong type for initializer (lambda function expected).");
      }
      else
      {
        throw std::string("Unkown type for initializer.");
      }
    }
  }

  const std::string &id = node->identifier();
  auto symbol = l22::make_symbol(node->type(), id, false, node->qualifier(), (bool)node->initializer(), false);
  if (_symtab.insert(id, symbol))
  {
    _parent->set_new_symbol(symbol);
  }
  else
  {
    throw std::string("variable '" + id + "' redeclared");
  }
}

//---------------------------------------------------------------------------

void l22::type_checker::do_nil_node(cdk::nil_node *const node, int lvl)
{
  // EMPTY
}

void l22::type_checker::do_again_node(l22::again_node *node, int lvl)
{
  // EMPTY
}

void l22::type_checker::do_stop_node(l22::stop_node *node, int lvl)
{
  // EMPTY
}

//---------------------------------------------------------------------------

void l22::type_checker::do_integer_node(cdk::integer_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void l22::type_checker::do_double_node(cdk::double_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
}

void l22::type_checker::do_string_node(cdk::string_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
}

void l22::type_checker::do_nullptr_node(l22::nullptr_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->type(cdk::reference_type::create(4, nullptr));
}

//---------------------------------------------------------------------------

void l22::type_checker::do_variable_node(cdk::variable_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  const std::string &id = node->name();
  std::shared_ptr<l22::symbol> symbol = _symtab.find(id);
  if (symbol)
  {
    node->type(symbol->type());
  }
  else
  {
    throw "Undeclared variable '" + id + "'.";
  }
}

void l22::type_checker::do_index_node(l22::index_node *node, int lvl)
{
  ASSERT_UNSPEC;

  node->base()->accept(this, lvl + 2);
  if (!node->base()->is_typed(cdk::TYPE_POINTER))
  {
    throw std::string("Index left-value must be a pointer.");
  }

  node->index()->accept(this, lvl + 2);
  if (node->index()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *input = dynamic_cast<l22::input_node *>(node->index());
    if (input != nullptr)
    {
      node->index()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }
  else if (!node->index()->is_typed(cdk::TYPE_INT))
  {
    throw std::string("Integer expression expected in left-value index.");
  }

  node->type(cdk::reference_type::cast(node->base()->type())->referenced());
}

void l22::type_checker::do_rvalue_node(cdk::rvalue_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->lvalue()->accept(this, lvl);
  node->type(node->lvalue()->type());
}

void l22::type_checker::do_assignment_node(cdk::assignment_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->lvalue()->accept(this, lvl + 2);
  node->rvalue()->accept(this, lvl + 2);
  if (node->lvalue()->is_typed(cdk::TYPE_UNSPEC))
  {
    throw std::string("Left value must have a type.");
  }

  if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *input = dynamic_cast<l22::input_node *>(node->rvalue());
    l22::stack_alloc_node *stack = dynamic_cast<l22::stack_alloc_node *>(node->rvalue());
    if (input != nullptr)
    {
      if (node->lvalue()->is_typed(cdk::TYPE_INT) || node->lvalue()->is_typed(cdk::TYPE_DOUBLE))
      {
        node->rvalue()->type(node->lvalue()->type());
      }
      else
      {
        throw std::string("Invalid expression for lvalue node.");
      }
    }
    else if (stack != nullptr)
    {
      if (node->lvalue()->is_typed(cdk::TYPE_POINTER))
      {
        node->rvalue()->type(node->lvalue()->type());
      }
      else
      {
        throw std::string("A pointer is required to allocate.");
      }
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }

  if (node->lvalue()->is_typed(cdk::TYPE_INT) && node->rvalue()->is_typed(cdk::TYPE_INT))
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
    l22::nullptr_node *n = dynamic_cast<l22::nullptr_node *>(node->rvalue());
    if (n == nullptr)
    {
      typeOfPointer(cdk::reference_type::cast(node->lvalue()->type()), cdk::reference_type::cast(node->rvalue()->type()));
    }
    node->type(node->lvalue()->type());
  }
  else if (node->lvalue()->is_typed(cdk::TYPE_FUNCTIONAL) && node->rvalue()->is_typed(cdk::TYPE_FUNCTIONAL))
  {
    // TODO: condição tem que ser mais forte do que isto
    node->type(node->lvalue()->type());
  }
  else
  {
    throw std::string("Wrong types in assignment.");
  }
}

//---------------------------------------------------------------------------

void l22::type_checker::do_UnaryExpression(cdk::unary_operation_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);
  if (node->argument()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *input = dynamic_cast<l22::input_node *>(node->argument());
    if (input != nullptr)
    {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
      node->argument()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
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
    throw std::string("Wrong type in argument of unary expression (Integer or double expected).");
  }
}

void l22::type_checker::do_neg_node(cdk::neg_node *const node, int lvl)
{
  do_UnaryExpression(node, lvl);
}

void l22::type_checker::do_not_node(cdk::not_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);
  if (node->argument()->is_typed(cdk::TYPE_INT))
  {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if (node->argument()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *input = dynamic_cast<l22::input_node *>(node->argument());
    if (input != nullptr)
    {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
      node->argument()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
      throw std::string("Unknown type in argument of unary expression (Integer expected).");
    }
  }
  else
  {
    throw std::string("Wrong type in argument of unary expression (Integer expected).");
  }
}

void l22::type_checker::do_identity_node(l22::identity_node *node, int lvl)
{
  do_UnaryExpression(node, lvl);
}

//---------------------------------------------------------------------------

void l22::type_checker::do_GeneralLogicalExpression(cdk::binary_operation_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);

  if (node->left()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *inputl = dynamic_cast<l22::input_node *>(node->left());
    if (inputl != nullptr)
    {
      node->left()->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }

  node->right()->accept(this, lvl + 2);
  if (node->right()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *inputr = dynamic_cast<l22::input_node *>(node->right());
    if (inputr != nullptr)
    {
      node->left()->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }

  if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_POINTER))
  {
    typeOfPointer(cdk::reference_type::cast(node->left()->type()), cdk::reference_type::cast(node->right()->type()));
  }
  else if (node->left()->type()->name() != node->right()->type()->name())
  {
    if (!((node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_DOUBLE)) || (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_INT))))
    {
      throw std::string("Operator has incompatible types.");
    }
  }
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void l22::type_checker::do_BooleanLogicalExpression(cdk::binary_operation_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);

  if (node->left()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *inputl = dynamic_cast<l22::input_node *>(node->left());

    if (inputl != nullptr)
    {
      node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }
  else if (!node->left()->is_typed(cdk::TYPE_INT))
  {
    throw std::string("Integer expression expected in (left and right) binary operators.");
  }

  node->right()->accept(this, lvl + 2);
  if (node->right()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *inputr = dynamic_cast<l22::input_node *>(node->right());

    if (inputr != nullptr)
    {
      node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }
  else if (!node->right()->is_typed(cdk::TYPE_INT))
  {
    throw std::string("Integer expression expected in (left and right) binary operators.");
  }

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void l22::type_checker::do_IntOnlyExpression(cdk::binary_operation_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  if (node->left()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *inputl = dynamic_cast<l22::input_node *>(node->left());

    if (inputl != nullptr)
    {
      node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }
  node->right()->accept(this, lvl + 2);
  if (node->left()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *inputl = dynamic_cast<l22::input_node *>(node->left());
    if (inputl != nullptr)
    {
      node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }
  if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_INT))
  {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else
  {
    throw std::string("Integer expression expected in (left and right) binary operators.");
  }
}

void l22::type_checker::do_ScalarLogicalExpression(cdk::binary_operation_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);

  if (node->left()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *inputl = dynamic_cast<l22::input_node *>(node->left());

    if (inputl != nullptr)
    {
      node->left()->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }
  else if (!node->left()->is_typed(cdk::TYPE_INT) && !node->left()->is_typed(cdk::TYPE_DOUBLE))
  {
    throw std::string("Wrong binary logical expression (expected integer or double).");
  }

  node->right()->accept(this, lvl + 2);
  if (node->right()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *inputr = dynamic_cast<l22::input_node *>(node->right());
    if (inputr != nullptr)
    {
      node->right()->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }
  else if (!node->right()->is_typed(cdk::TYPE_INT) && !node->right()->is_typed(cdk::TYPE_DOUBLE))
  {
    throw std::string("Wrong binary logical expression (expected integer or double).");
  }

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void l22::type_checker::do_IDExpression(cdk::binary_operation_node *const node, int lvl)
{
  ASSERT_UNSPEC;
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
    l22::input_node *inputl = dynamic_cast<l22::input_node *>(node->left());
    l22::input_node *inputr = dynamic_cast<l22::input_node *>(node->right());

    if (inputl != nullptr && inputr != nullptr)
    {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
      node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
      node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }
  else if (node->left()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *inputl = dynamic_cast<l22::input_node *>(node->left());
    if (inputl != nullptr)
    {
      node->left()->type(node->right()->type());
      node->type(node->right()->type());
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }
  else if (node->right()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *inputr = dynamic_cast<l22::input_node *>(node->right());

    if (inputr != nullptr)
    {
      node->right()->type(node->left()->type());
      node->type(node->left()->type());
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }
  else
  {
    throw std::string("Wrong types in binary expression.");
  }
}

//---------------------------------------------------------------------------

void l22::type_checker::do_add_node(cdk::add_node *const node, int lvl)
{
  ASSERT_UNSPEC;
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
      throw std::string("Pointer arithmetic not supported for function pointers.");
    }
    node->type(node->left()->type());
  }
  else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_POINTER))
  {
    std::shared_ptr<cdk::reference_type> reference_type = cdk::reference_type::cast(node->right()->type());
    if (reference_type->referenced()->name() == cdk::TYPE_FUNCTIONAL)
    {
      throw std::string("Pointer arithmetic not supported for function pointers.");
    }
    node->type(node->right()->type());
  }
  else if (node->left()->is_typed(cdk::TYPE_UNSPEC) && node->right()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *inputl = dynamic_cast<l22::input_node *>(node->left());
    if (inputl != nullptr)
    {
      node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
    l22::input_node *inputr = dynamic_cast<l22::input_node *>(node->right());
    if (inputr != nullptr)
    {
      node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if (node->left()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *inputl = dynamic_cast<l22::input_node *>(node->left());
    if (inputl != nullptr)
    {
      if (node->right()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_INT))
      {
        node->left()->type(node->right()->type());
      }
      else
      {
        throw std::string("Invalid expression in right argument of binary expression.");
      }
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }
  else if (node->right()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *inputr = dynamic_cast<l22::input_node *>(node->right());
    if (inputr != nullptr)
    {
      if (node->left()->is_typed(cdk::TYPE_DOUBLE) || node->left()->is_typed(cdk::TYPE_INT))
      {
        node->right()->type(node->left()->type());
      }
      else
      {
        throw std::string("Invalid expression in left argument of binary expression.");
      }
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }
  else
  {
    throw std::string("Wrong types in binary expression.");
  }
}

void l22::type_checker::do_sub_node(cdk::sub_node *const node, int lvl)
{
  ASSERT_UNSPEC;
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
      throw std::string("Pointer arithmetic not supported for function pointers.");
    }
    node->type(node->left()->type());
  }
  else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_POINTER))
  {
    std::shared_ptr<cdk::reference_type> reference_type = cdk::reference_type::cast(node->right()->type());
    if (reference_type->referenced()->name() == cdk::TYPE_FUNCTIONAL)
    {
      throw std::string("Pointer arithmetic not supported for function pointers.");
    }
    node->type(node->right()->type());
  }
  else if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_POINTER))
  {
    std::shared_ptr<cdk::reference_type> reference_type_left = cdk::reference_type::cast(node->left()->type());
    if (reference_type_left->referenced()->name() == cdk::TYPE_FUNCTIONAL)
    {
      throw std::string("Pointer arithmetic not supported for function pointers.");
    }
    std::shared_ptr<cdk::reference_type> reference_type_right = cdk::reference_type::cast(node->right()->type());
    if (reference_type_right->referenced()->name() == cdk::TYPE_FUNCTIONAL)
    {
      throw std::string("Pointer arithmetic not supported for function pointers.");
    }
    typeOfPointer(reference_type_left, reference_type_right);
    node->type(node->left()->type());
  }
  else if (node->left()->is_typed(cdk::TYPE_UNSPEC) && node->right()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *inputl = dynamic_cast<l22::input_node *>(node->left());
    l22::input_node *inputr = dynamic_cast<l22::input_node *>(node->right());

    if (inputl != nullptr)
    {
      node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }

    if (inputr != nullptr)
    {
      node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if (node->left()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *inputl = dynamic_cast<l22::input_node *>(node->left());

    if (inputl != nullptr)
    {
      if (node->right()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_INT))
      {
        node->left()->type(node->right()->type());
      }
      else
      {
        throw std::string("Invalid expression in right argument of binary expression.");
      }
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }
  else if (node->right()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *inputr = dynamic_cast<l22::input_node *>(node->right());

    if (inputr != nullptr)
    {
      if (node->left()->is_typed(cdk::TYPE_DOUBLE) || node->left()->is_typed(cdk::TYPE_INT))
      {
        node->right()->type(node->left()->type());
      }
      else
      {
        throw std::string("Invalid expression in left argument of binary expression.");
      }
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }
  else
  {
    throw std::string("Wrong types in binary expression.");
  }
}

void l22::type_checker::do_mul_node(cdk::mul_node *const node, int lvl)
{
  do_IDExpression(node, lvl);
}

void l22::type_checker::do_div_node(cdk::div_node *const node, int lvl)
{
  do_IDExpression(node, lvl);
}

void l22::type_checker::do_mod_node(cdk::mod_node *const node, int lvl)
{
  do_IntOnlyExpression(node, lvl);
}

//---------------------------------------------------------------------------

void l22::type_checker::do_gt_node(cdk::gt_node *const node, int lvl)
{
  do_ScalarLogicalExpression(node, lvl);
}

void l22::type_checker::do_ge_node(cdk::ge_node *const node, int lvl)
{
  do_ScalarLogicalExpression(node, lvl);
}

void l22::type_checker::do_le_node(cdk::le_node *const node, int lvl)
{
  do_ScalarLogicalExpression(node, lvl);
}

void l22::type_checker::do_lt_node(cdk::lt_node *const node, int lvl)
{
  do_ScalarLogicalExpression(node, lvl);
}

void l22::type_checker::do_eq_node(cdk::eq_node *const node, int lvl)
{
  do_GeneralLogicalExpression(node, lvl);
}

void l22::type_checker::do_ne_node(cdk::ne_node *const node, int lvl)
{
  do_GeneralLogicalExpression(node, lvl);
}

void l22::type_checker::do_and_node(cdk::and_node *const node, int lvl)
{
  do_BooleanLogicalExpression(node, lvl);
}

void l22::type_checker::do_or_node(cdk::or_node *const node, int lvl)
{
  do_BooleanLogicalExpression(node, lvl);
}

//---------------------------------------------------------------------------

void l22::type_checker::do_evaluation_node(l22::evaluation_node *const node, int lvl)
{
  node->argument()->accept(this, lvl + 2);

  if (node->argument()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *input = dynamic_cast<l22::input_node *>(node->argument());

    if (input != nullptr)
    {
      node->argument()->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }
}

void l22::type_checker::do_print_node(l22::print_node *const node, int lvl)
{
  node->arguments()->accept(this, lvl + 2);
}

void l22::type_checker::do_input_node(l22::input_node *node, int lvl)
{
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(0, cdk::TYPE_UNSPEC));
}

//---------------------------------------------------------------------------

void l22::type_checker::do_address_of_node(l22::address_of_node *node, int lvl)
{
  ASSERT_UNSPEC;
  node->lvalue()->accept(this, lvl + 2);
  node->type(cdk::reference_type::create(4, node->lvalue()->type()));
}

void l22::type_checker::do_stack_alloc_node(l22::stack_alloc_node *node, int lvl)
{
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);
  if (!node->argument()->is_typed(cdk::TYPE_INT))
  {
    throw std::string("integer expression expected in allocation expression");
  }
  auto mytype = cdk::reference_type::create(4, cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  node->type(mytype);
}

//---------------------------------------------------------------------------

void l22::type_checker::do_while_node(l22::while_node *const node, int lvl)
{
  node->condition()->accept(this, lvl + 2);
  node->condition()->accept(this, lvl + 2);
}

//---------------------------------------------------------------------------

void l22::type_checker::do_if_node(l22::if_node *const node, int lvl)
{
  node->condition()->accept(this, lvl + 2);
  if (node->condition()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *input = dynamic_cast<l22::input_node *>(node->condition());

    if (input != nullptr)
    {
      node->condition()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }
  if (!node->condition()->is_typed(cdk::TYPE_INT))
  {
    throw std::string("Expected integer condition");
  }
}

void l22::type_checker::do_if_else_node(l22::if_else_node *const node, int lvl)
{
  node->condition()->accept(this, lvl + 2);
  if (node->condition()->is_typed(cdk::TYPE_UNSPEC))
  {
    l22::input_node *input = dynamic_cast<l22::input_node *>(node->condition());

    if (input != nullptr)
    {
      node->condition()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
      throw std::string("Unknown node with unspecified type.");
    }
  }
  if (!node->condition()->is_typed(cdk::TYPE_INT))
  {
    throw std::string("Expected integer condition");
  }
}

//---------------------------------------------------------------------------

void l22::type_checker::do_function_call_node(l22::function_call_node *node, int lvl)
{
  ASSERT_UNSPEC;
  std::shared_ptr<cdk::functional_type> fType;

  cdk::variable_node *variable = dynamic_cast<cdk::variable_node *>(node->lambda_ptr());
  if (variable != nullptr)
  {
    if (variable->name() == "@")
    {
      if (_lambda_stack.size())
      {
        fType = _lambda_stack.top();
      }
      else
      {
        throw std::string("Recursive call outside of function is not allowed.");
      }
    }
    else
    {
      variable->accept(this, lvl + 2);
      if (!variable->is_typed(cdk::TYPE_FUNCTIONAL))
      {
        throw std::string("Left argument is not of functional type.");
      }
      fType = cdk::functional_type::cast(node->lambda_ptr()->type());
    }
  }
  else
  {
    node->lambda_ptr()->accept(this, lvl + 2);
    if (!node->lambda_ptr()->is_typed(cdk::TYPE_FUNCTIONAL))
    {
      throw std::string("Left argument is not of functional type.");
    }
    fType = cdk::functional_type::cast(node->lambda_ptr()->type());
  }

  node->type(fType->output());
  if (node->arguments())
  {
    node->arguments()->accept(this, lvl + 2);
    if (fType->input_length() != node->arguments()->size())
    {
      throw std::string("Mismatching number of arguments in function call.");
    }
    for (size_t i = 0; i < fType->input_length(); i++)
    {
      if (!(node->argument(i)->is_typed(fType->input(i)->name())))
      {
        throw std::string("Type mismatch for arguments in function call.");
      }
      if (node->argument(i)->is_typed(cdk::TYPE_INT) && (fType->input(i)->name() == cdk::TYPE_DOUBLE))
      {
        throw std::string("Type mismatch for arguments in function call.");
      }
    }
  }
}

void l22::type_checker::do_lambda_node(l22::lambda_node *node, int lvl)
{
  ASSERT_UNSPEC;
  std::vector<std::shared_ptr<cdk::basic_type>> *argsTypes = new std::vector<std::shared_ptr<cdk::basic_type>>();
  if (node->arguments())
  {
    for (size_t i = 0; i < node->arguments()->size(); i++)
    {
      argsTypes->push_back(node->argument(i)->type());
    }
  }
  if (node->block())
  {
    _lambda_stack.push(cdk::functional_type::create(*argsTypes, node->type()));
    node->block()->accept(this, lvl + 2);
    _lambda_stack.pop();
  }
}

//---------------------------------------------------------------------------

void l22::type_checker::do_sizeof_node(l22::sizeof_node *node, int lvl)
{
  ASSERT_UNSPEC;
  node->expression()->accept(this, lvl + 2);
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}
