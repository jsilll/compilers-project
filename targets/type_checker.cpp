#include <string>
#include "targets/type_checker.h"
#include ".auto/all_nodes.h"
#include <cdk/types/primitive_type.h>

#define ASSERT_UNSPEC                                                 \
  {                                                                   \
    if (node->type() != nullptr && !node->is_typed(cdk::TYPE_UNSPEC)) \
      return;                                                         \
  }

//---------------------------------------------------------------------------

void l22::type_checker::do_program_node(l22::program_node *const node, int lvl)
{
  if (node->block())
  {
    node->block()->accept(this, lvl + 2);
  }
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
  if (node->declarations())
  {
    node->declarations()->accept(this, lvl + 2);
  }
  else if (node->instructions())
  {
    node->instructions()->accept(this, lvl + 2);
  }
}

void l22::type_checker::do_return_node(l22::return_node *node, int lvl)
{
  if (node->retval())
  {
    if (_function->type() != nullptr && _function->is_typed(cdk::TYPE_VOID))
    {
      throw std::string("Void function cannot return values.");
    }

    node->retval()->accept(this, lvl + 2);

    if (_function->is_typed(cdk::TYPE_INT) && !node->retval()->is_typed(cdk::TYPE_INT))
    {
      throw std::string("Wrong type for initializer (integer expected).");
    }
    else if (_function->is_typed(cdk::TYPE_DOUBLE) && (!node->retval()->is_typed(cdk::TYPE_INT) && !node->retval()->is_typed(cdk::TYPE_DOUBLE)))
    {
      throw std::string("Wrong type for initializer (integer or double expected).");
    }
    else if (_function->is_typed(cdk::TYPE_STRING) && !node->retval()->is_typed(cdk::TYPE_STRING))
    {
      throw std::string("Wrong type for initializer (string expected).");
    }
    else if (_function->is_typed(cdk::TYPE_POINTER))
    {
      // TODO: 2 alternativas??
      // typeOfPointer(cdk::reference_type_cast(node->retval()->type()), cdk::reference_type_cast(_function->type()));
    }
    else
    {
      throw std::string("Unknown type for return expression.");
    }
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
  std::shared_ptr<cdk::reference_type> btype;

  if (node->base())
  {
    node->base()->accept(this, lvl + 2);
    btype = cdk::reference_type::cast(node->base()->type());
    if (!node->base()->is_typed(cdk::TYPE_POINTER))
    {
      throw std::string("Index left-value must be a pointer.");
    }
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
  // TODO
}

//---------------------------------------------------------------------------

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

void l22::type_checker::do_UnaryExpression(cdk::unary_operation_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);
  if (node->argument()->is_typed(cdk::TYPE_INT))
  {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if (node->argument()->is_typed(cdk::TYPE_DOUBLE))
  {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
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
      throw std::string("Unknown node with unspecified type.");
    }
  }
  else
  {
    throw std::string("Wrong type in argument of unary expression (Integer or double expected).");
  }
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
    // TODO: typeOfPointer(cdk::reference_type_cast(node->left()->type()), cdk::reference_type_cast(node->right()->type()));
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
    node->type(node->left()->type());
  }
  else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_POINTER))
  {
    node->type(node->right()->type());
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

void l22::type_checker::do_sub_node(cdk::sub_node *const node, int lvl)
{
  do_BinaryExpression(node, lvl);
}

void l22::type_checker::do_mul_node(cdk::mul_node *const node, int lvl)
{
  do_BinaryExpression(node, lvl);
}

void l22::type_checker::do_div_node(cdk::div_node *const node, int lvl)
{
  do_BinaryExpression(node, lvl);
}

void l22::type_checker::do_mod_node(cdk::mod_node *const node, int lvl)
{
  do_BinaryExpression(node, lvl);
}

//---------------------------------------------------------------------------

void l22::type_checker::do_gt_node(cdk::gt_node *const node, int lvl)
{
  do_BinaryExpression(node, lvl);
}

void l22::type_checker::do_ge_node(cdk::ge_node *const node, int lvl)
{
  do_BinaryExpression(node, lvl);
}

void l22::type_checker::do_le_node(cdk::le_node *const node, int lvl)
{
  do_BinaryExpression(node, lvl);
}

void l22::type_checker::do_lt_node(cdk::lt_node *const node, int lvl)
{
  do_BinaryExpression(node, lvl);
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
  do_BinaryExpression(node, lvl);
}

void l22::type_checker::do_or_node(cdk::or_node *const node, int lvl)
{
  do_BinaryExpression(node, lvl);
}

//---------------------------------------------------------------------------

void l22::type_checker::do_evaluation_node(l22::evaluation_node *const node, int lvl)
{
  node->argument()->accept(this, lvl + 2);
}

void l22::type_checker::do_print_node(l22::print_node *const node, int lvl)
{
  node->arguments()->accept(this, lvl + 2);
}

void l22::type_checker::do_input_node(l22::input_node *node, int lvl)
{
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
  node->condition()->accept(this, lvl + 4);
}

//---------------------------------------------------------------------------

void l22::type_checker::do_if_node(l22::if_node *const node, int lvl)
{
  node->condition()->accept(this, lvl + 4);
  if (!node->condition()->is_typed(cdk::TYPE_INT))
    throw std::string("expected integer condition");
}

void l22::type_checker::do_if_else_node(l22::if_else_node *const node, int lvl)
{
  node->condition()->accept(this, lvl + 4);
  if (!node->condition()->is_typed(cdk::TYPE_INT))
    throw std::string("expected integer condition");
}

//---------------------------------------------------------------------------

void l22::type_checker::do_function_call_node(l22::function_call_node *node, int lvl)
{
}

void l22::type_checker::do_lambda_node(l22::lambda_node *node, int lvl)
{
}

//---------------------------------------------------------------------------

void l22::type_checker::do_sizeof_node(l22::sizeof_node *node, int lvl)
{
  ASSERT_UNSPEC;
  node->expression()->accept(this, lvl + 2);
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}
