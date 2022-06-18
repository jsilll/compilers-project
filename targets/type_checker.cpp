#include <string>
#include "targets/type_checker.h"
#include ".auto/all_nodes.h" // automatically generated
#include <cdk/types/primitive_type.h>

#define ASSERT_UNSPEC                                                 \
  {                                                                   \
    if (node->type() != nullptr && !node->is_typed(cdk::TYPE_UNSPEC)) \
      return;                                                         \
  }

//---------------------------------------------------------------------------

void l22::type_checker::do_sequence_node(cdk::sequence_node *const node, int lvl)
{
  for (size_t i = 0; i < node->size(); i++)
    node->node(i)->accept(this, lvl);
}

//---------------------------------------------------------------------------

void l22::type_checker::do_nil_node(cdk::nil_node *const node, int lvl)
{
  // EMPTY
}
void l22::type_checker::do_data_node(cdk::data_node *const node, int lvl)
{
  // EMPTY
}

//---------------------------------------------------------------------------

/* data type nodes */

void l22::type_checker::do_integer_node(cdk::integer_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void l22::type_checker::do_string_node(cdk::string_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
}

void l22::type_checker::do_double_node(cdk::double_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_DOUBLE));
}

void l22::type_checker::do_nullptr_node(l22::nullptr_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->type(cdk::reference_type::create(4, nullptr));
}

//---------------------------------------------------------------------------

/* unary expression nodes */

void l22::type_checker::processUnaryExpression(cdk::unary_operation_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);
  if (node->argument()->is_typed(cdk::TYPE_INT))
  {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if (node->argument()->is_typed(cdk::TYPE_UNSPEC))
  {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    node->argument()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else
  {
    throw std::string("wrong type in unary logical expression");
  }
}

void l22::type_checker::do_neg_node(cdk::neg_node *const node, int lvl)
{
  processUnaryExpression(node, lvl);
}

void l22::type_checker::do_not_node(cdk::not_node *const node, int lvl)
{
  processUnaryExpression(node, lvl);
}

void l22::type_checker::do_identity_node(l22::identity_node *node, int lvl)
{
  processUnaryExpression(node, lvl);
}

//---------------------------------------------------------------------------

void l22::type_checker::processBinaryExpression(cdk::binary_operation_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  if (!node->left()->is_typed(cdk::TYPE_INT))
    throw std::string("wrong type in left argument of binary expression");

  node->right()->accept(this, lvl + 2);
  if (!node->right()->is_typed(cdk::TYPE_INT))
    throw std::string("wrong type in right argument of binary expression");

  // in Simple, expressions are always int
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void l22::type_checker::do_GeneralLogicalExpression(cdk::binary_operation_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);
  if (node->left()->type() != node->right()->type())
  {
    throw std::string("same type expected on both sides of equality operator");
  }
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

/* arithmetic nodes */

void l22::type_checker::do_add_node(cdk::add_node *const node, int lvl)
{
  processBinaryExpression(node, lvl);
}
void l22::type_checker::do_sub_node(cdk::sub_node *const node, int lvl)
{
  processBinaryExpression(node, lvl);
}
void l22::type_checker::do_mul_node(cdk::mul_node *const node, int lvl)
{
  processBinaryExpression(node, lvl);
}
void l22::type_checker::do_div_node(cdk::div_node *const node, int lvl)
{
  processBinaryExpression(node, lvl);
}
void l22::type_checker::do_mod_node(cdk::mod_node *const node, int lvl)
{
  processBinaryExpression(node, lvl);
}

/* comparison nodes */

// check if i want to change binary expression checking

void l22::type_checker::do_and_node(cdk::and_node *const node, int lvl)
{
  processBinaryExpression(node, lvl);
}
void l22::type_checker::do_or_node(cdk::or_node *const node, int lvl)
{
  processBinaryExpression(node, lvl);
}

void l22::type_checker::do_lt_node(cdk::lt_node *const node, int lvl)
{
  processBinaryExpression(node, lvl);
}
void l22::type_checker::do_le_node(cdk::le_node *const node, int lvl)
{
  processBinaryExpression(node, lvl);
}
void l22::type_checker::do_ge_node(cdk::ge_node *const node, int lvl)
{
  processBinaryExpression(node, lvl);
}
void l22::type_checker::do_gt_node(cdk::gt_node *const node, int lvl)
{
  processBinaryExpression(node, lvl);
}
void l22::type_checker::do_ne_node(cdk::ne_node *const node, int lvl)
{
  do_GeneralLogicalExpression(node, lvl);
}
void l22::type_checker::do_eq_node(cdk::eq_node *const node, int lvl)
{
  do_GeneralLogicalExpression(node, lvl);
}

//---------------------------------------------------------------------------

/* all variable relation nodes */

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
    throw id;
  }
}

void l22::type_checker::do_rvalue_node(cdk::rvalue_node *const node, int lvl)
{
  ASSERT_UNSPEC;
  try
  {
    node->lvalue()->accept(this, lvl);
    node->type(node->lvalue()->type());
  }
  catch (const std::string &id)
  {
    throw "undeclared variable '" + id + "'";
  }
}

void l22::type_checker::do_assignment_node(cdk::assignment_node *const node, int lvl)
{
  ASSERT_UNSPEC;

  node->lvalue()->accept(this, lvl + 4);
  node->rvalue()->accept(this, lvl + 4);

  if (node->lvalue()->is_typed(cdk::TYPE_INT))
  {
    if (node->rvalue()->is_typed(cdk::TYPE_INT))
    {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC))
    {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
      node->rvalue()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }
    else
    {
      throw std::string("wrong assignment to integer");
    }
  }
  else if (node->lvalue()->is_typed(cdk::TYPE_POINTER))
  {

    // TODO: check pointer level

    if (node->rvalue()->is_typed(cdk::TYPE_POINTER))
    {
      node->type(node->rvalue()->type());
    }
    else if (node->rvalue()->is_typed(cdk::TYPE_INT))
    {
      // TODO: check that the integer is a literal and that it is zero
      node->type(cdk::primitive_type::create(4, cdk::TYPE_POINTER));
    }
    else if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC))
    {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_ERROR));
      node->rvalue()->type(cdk::primitive_type::create(4, cdk::TYPE_ERROR));
    }
    else
    {
      throw std::string("wrong assignment to pointer");
    }
  }
  else if (node->lvalue()->is_typed(cdk::TYPE_DOUBLE))
  {

    if (node->rvalue()->is_typed(cdk::TYPE_DOUBLE) || node->rvalue()->is_typed(cdk::TYPE_INT))
    {
      node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
    }
    else if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC))
    {
      node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
      node->rvalue()->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
    }
    else
    {
      throw std::string("wrong assignment to real");
    }
  }
  else if (node->lvalue()->is_typed(cdk::TYPE_STRING))
  {

    if (node->rvalue()->is_typed(cdk::TYPE_STRING))
    {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
    }
    else if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC))
    {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
      node->rvalue()->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
    }
    else
    {
      throw std::string("wrong assignment to string");
    }
  }
  else
  {
    throw std::string("wrong types in assignment");
  }
}

void l22::type_checker::do_declaration_node(l22::declaration_node *node, int lvl)
{
  if (node->initializer() != nullptr)
  {
    node->initializer()->accept(this, lvl + 2);

    if (node->is_typed(cdk::TYPE_INT))
    {
      if (!node->initializer()->is_typed(cdk::TYPE_INT))
        throw std::string("wrong type for initializer (integer expected).");
    }
    else if (node->is_typed(cdk::TYPE_DOUBLE))
    {
      if (!node->initializer()->is_typed(cdk::TYPE_INT) && !node->initializer()->is_typed(cdk::TYPE_DOUBLE))
      {
        throw std::string("wrong type for initializer (integer or double expected).");
      }
    }
    else if (node->is_typed(cdk::TYPE_STRING))
    {
      if (!node->initializer()->is_typed(cdk::TYPE_STRING))
      {
        throw std::string("wrong type for initializer (string expected).");
      }
    }
    else if (node->is_typed(cdk::TYPE_POINTER))
    {
      // DAVID: FIXME: trouble!!!
      if (!node->initializer()->is_typed(cdk::TYPE_POINTER))
      {
        auto in = (cdk::literal_node<int> *)node->initializer();
        if (in == nullptr || in->value() != 0)
          throw std::string("wrong type for initializer (pointer expected).");
      }
    }
    else
    {
      throw std::string("unknown type for initializer.");
    }

    const std::string &id = node->identifier();
    auto symbol = l22::make_symbol(false, node->qualifier(), node->type(), id, (bool)node->initializer());
    if (_symtab.insert(id, symbol))
    {
      _parent->set_new_symbol(symbol); // advise parent that a symbol has been inserted
    }
    else
    {
      throw std::string("variable '" + id + "' redeclared");
    }
  }
}

//---------------------------------------------------------------------------

void l22::type_checker::do_program_node(l22::program_node *const node, int lvl)
{
  // EMPTY
}

void l22::type_checker::do_evaluation_node(l22::evaluation_node *const node, int lvl)
{
  node->argument()->accept(this, lvl + 2);
}

void l22::type_checker::do_print_node(l22::print_node *const node, int lvl)
{
  node->arguments()->accept(this, lvl + 2);
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
}

void l22::type_checker::do_if_else_node(l22::if_else_node *const node, int lvl)
{
  node->condition()->accept(this, lvl + 4);
}

//---------------------------------------------------------------------------

void l22::type_checker::do_again_node(l22::again_node *node, int lvl)
{
}

//---------------------------------------------------------------------------

void l22::type_checker::do_block_node(l22::block_node *node, int lvl)
{
}

//---------------------------------------------------------------------------

void l22::type_checker::do_function_call_node(l22::function_call_node *node, int lvl)
{
}

//---------------------------------------------------------------------------

void l22::type_checker::do_lambda_node(l22::lambda_node *node, int lvl)
{
}

//---------------------------------------------------------------------------

void l22::type_checker::do_return_node(l22::return_node *node, int lvl)
{
}

//---------------------------------------------------------------------------

void l22::type_checker::do_stop_node(l22::stop_node *node, int lvl)
{
}

//---------------------------------------------------------------------------

void l22::type_checker::do_address_of_node(l22::address_of_node *node, int lvl)
{
}

//---------------------------------------------------------------------------

void l22::type_checker::do_index_node(l22::index_node *node, int lvl)
{
}

//---------------------------------------------------------------------------

void l22::type_checker::do_input_node(l22::input_node *node, int lvl)
{
}

//---------------------------------------------------------------------------

void l22::type_checker::do_sizeof_node(l22::sizeof_node *node, int lvl)
{
}

//---------------------------------------------------------------------------

void l22::type_checker::do_stack_alloc_node(l22::stack_alloc_node *node, int lvl)
{
}
