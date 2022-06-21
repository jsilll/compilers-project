#include <string>
#include <sstream>
#include "targets/type_checker.h"
#include "targets/postfix_writer.h"
#include "targets/frame_size_calculator.h"
#include ".auto/all_nodes.h"

#include "l22_parser.tab.h"

//---------------------------------------------------------------------------

void l22::postfix_writer::do_program_node(l22::program_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_program_node(l22::program_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;

  _function.push(new_lambda());
  reset_new_lambda();

  // generate the main function (RTS mandates that its name be "_main")
  _pf.TEXT();
  _pf.ALIGN();
  _pf.GLOBAL("_main", _pf.FUNC());
  _pf.LABEL("_main");

  frame_size_calculator lsc(_compiler, _symtab);
  node->accept(&lsc, lvl);
  _pf.ENTER(lsc.localsize()); // total stack size reserved for local variables

  _inFunctionBody = true;
  node->block()->accept(this, lvl);
  _inFunctionBody = false;

  _function.pop();

  _pf.LEAVE();
  _pf.RET();

  // these are just a few library function imports
  for (std::string s : _functions_to_declare)
  {
    _pf.EXTERN(s);
  }
}

//---------------------------------------------------------------------------

void l22::postfix_writer::do_data_node(cdk::data_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_data_node(cdk::data_node *const node, int lvl)" << std::endl;
  // EMPTY
}

void l22::postfix_writer::do_nil_node(cdk::nil_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_nil_node(cdk::nil_node *const node, int lvl)" << std::endl;
}

void l22::postfix_writer::do_sequence_node(cdk::sequence_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_sequence_node(cdk::sequence_node *const node, int lvl)" << std::endl;
  for (size_t i = 0; i < node->size(); i++)
  {
    node->node(i)->accept(this, lvl);
  }
}

void l22::postfix_writer::do_return_node(l22::return_node *node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_return_node(l22::return_node *node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;

  auto ret_type = _function.top()->output()->component(0)->name();

  // ver como retornar com functional types
  if (!(ret_type == cdk::TYPE_VOID))
  {
    node->retval()->accept(this, lvl + 2);

    if (ret_type == cdk::TYPE_INT || ret_type == cdk::TYPE_STRING || ret_type == cdk::TYPE_POINTER || ret_type == cdk::TYPE_FUNCTIONAL)
      _pf.STFVAL32();
    else if (ret_type == cdk::TYPE_DOUBLE)
    {
      cdk::expression_node *expression = dynamic_cast<cdk::expression_node *>(node->retval());
      if (expression->is_typed(cdk::TYPE_INT))
        _pf.I2D();
      _pf.STFVAL64();
    }
    else
      std::cerr << node->lineno() << ": Unknown types for function return instruction." << std::endl;
  }

  _pf.LEAVE();
  _pf.RET();
}

void l22::postfix_writer::do_declaration_node(l22::declaration_node *node, int lvl)
{
  ASSERT_SAFE_EXPRESSIONS;
  std::cout << "void l22::postfix_writer::do_declaration_node(l22::declaration_node *node, int lvl)" << std::endl;

  std::string id = node->identifier();

  int offset = 0;
  int size = node->type()->size();
  if (_inFunctionBody)
  {
    _offset -= size;
    offset = _offset;
  }
  else if (_inFunctionArgs)
  {
    offset = _offset;
    _offset += size;
  }
  else
    offset = 0;

  std::shared_ptr<l22::symbol> symbol = new_symbol();

  if (symbol)
  {
    symbol->set_offset(offset);
    reset_new_symbol();
  }

  if (node->initializer())
  {
    if (_inFunctionBody)
    {
      node->initializer()->accept(this, lvl);
      if (node->is_typed(cdk::TYPE_DOUBLE))
      {
        if (node->initializer()->is_typed(cdk::TYPE_INT))
          _pf.I2D();

        _pf.LOCAL(offset);
        _pf.STDOUBLE();
      }
      else
      {
        _pf.LOCAL(offset);
        _pf.STINT();
      }
    }
    else if (!_inFunctionBody && !_inFunctionArgs)
    {
      if (node->is_typed(cdk::TYPE_INT) || node->is_typed(cdk::TYPE_POINTER) || node->is_typed(cdk::TYPE_DOUBLE))
      {
        _pf.DATA();
        _pf.ALIGN();
        if (node->qualifier() == tPUBLIC)
        {
          _pf.GLOBAL(id, _pf.OBJ());
        }
        _pf.LABEL(id);
        if (node->is_typed(cdk::TYPE_INT) || node->is_typed(cdk::TYPE_POINTER))
        {
          node->initializer()->accept(this, lvl);
        }
        else if (node->is_typed(cdk::TYPE_DOUBLE))
        {
          if (node->initializer()->is_typed(cdk::TYPE_DOUBLE))
            node->initializer()->accept(this, lvl);
          else if (node->initializer()->is_typed(cdk::TYPE_INT))
          {
            cdk::integer_node *dclini = dynamic_cast<cdk::integer_node *>(node->initializer());
            cdk::double_node ddi(dclini->lineno(), dclini->value());
            ddi.accept(this, lvl);
          }
          else
          {
            std::cerr << node->lineno() << ": '" << id << "' wrong initializer for real variable.\n";
            exit(2);
          }
        }
      }
      else if (node->is_typed(cdk::TYPE_STRING))
      {
        _pf.DATA();
        _pf.ALIGN();
        _pf.LABEL(node->identifier());
        node->initializer()->accept(this, lvl);
      }
    }
    else
    {
      std::cerr << node->lineno() << ": '" << id << "' has an unexpected initializer.\n";
      exit(2);
    }
  }
  else
  {
    if (!_inFunctionBody && !_inFunctionArgs && (node->is_typed(cdk::TYPE_INT) || node->is_typed(cdk::TYPE_POINTER) || node->is_typed(cdk::TYPE_DOUBLE) || node->is_typed(cdk::TYPE_STRING)))
    {
      _pf.BSS();
      _pf.ALIGN();
      _pf.LABEL(id);
      _pf.SALLOC(size);
    }
  }
}

//---------------------------------------------------------------------------

void l22::postfix_writer::do_double_node(cdk::double_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_double_node(cdk::double_node *const node, int lvl)" << std::endl;
  if (_inFunctionBody)
  {
    _pf.DOUBLE(node->value());
  }
  else
  {
    _pf.SDOUBLE(node->value());
  }
}

void l22::postfix_writer::do_integer_node(cdk::integer_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_integer_node(cdk::integer_node *const node, int lvl)" << std::endl;
  if (_inFunctionBody)
    _pf.INT(node->value());
  else
    _pf.SINT(node->value());
}

void l22::postfix_writer::do_string_node(cdk::string_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_string_node(cdk::string_node *const node, int lvl)" << std::endl;
  int lbl1;

  _pf.RODATA();
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl1 = ++_lbl));
  _pf.SSTRING(node->value());

  _pf.ALIGN();
  if (_inFunctionBody)
  {
    _pf.TEXT();
    _pf.ADDR(mklbl(lbl1));
  }
  else
  {
    _pf.DATA();
    _pf.SADDR(mklbl(lbl1));
  }
}

//---------------------------------------------------------------------------

void l22::postfix_writer::do_variable_node(cdk::variable_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_variable_node(cdk::variable_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  if (_inFunctionBody)
  {
    std::shared_ptr<l22::symbol> var = _symtab.find(node->name());
    if (var->offset() == 0)
      _pf.ADDR(var->name());
    else
      _pf.LOCAL(var->offset());
  }
  else
    _pf.ADDR(node->name());
}

void l22::postfix_writer::do_index_node(l22::index_node *node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_index_node(l22::index_node *node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  node->base()->accept(this, lvl);
  node->index()->accept(this, lvl);
  _pf.INT(node->type()->size());
  _pf.MUL();
  _pf.ADD();
}

void l22::postfix_writer::do_rvalue_node(cdk::rvalue_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_rvalue_node(cdk::rvalue_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  node->lvalue()->accept(this, lvl);
  if (node->is_typed(cdk::TYPE_INT) || node->is_typed(cdk::TYPE_POINTER) ||
      node->is_typed(cdk::TYPE_STRING))
    _pf.LDINT();
  else if (node->is_typed(cdk::TYPE_DOUBLE))
    _pf.LDDOUBLE();
}

void l22::postfix_writer::do_assignment_node(cdk::assignment_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_assignment_node(cdk::assignment_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  node->rvalue()->accept(this, lvl);
  if (node->is_typed(cdk::TYPE_DOUBLE))
  {
    if (node->rvalue()->is_typed(cdk::TYPE_INT))
      _pf.I2D();
    _pf.DUP64();
  }
  else
    _pf.DUP32();

  node->lvalue()->accept(this, lvl);
  if (node->lvalue()->is_typed(cdk::TYPE_DOUBLE))
  {
    _pf.STDOUBLE();
  }
  else
  {
    _pf.STINT();
  }
}

//---------------------------------------------------------------------------

void l22::postfix_writer::do_input_node(l22::input_node *node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_input_node(l22::input_node *node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  if (node->is_typed(cdk::TYPE_INT))
  {
    _functions_to_declare.insert("readi");
    _pf.CALL("readi");
    _pf.LDFVAL32();
  }
  else if (node->is_typed(cdk::TYPE_DOUBLE))
  {
    _functions_to_declare.insert("readd");
    _pf.CALL("readd");
    _pf.LDFVAL64();
  }
}

void l22::postfix_writer::do_print_node(l22::print_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_print_node(l22::print_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  for (size_t ix = 0; ix < node->arguments()->size(); ix++)
  {
    auto child = dynamic_cast<cdk::expression_node *>(node->arguments()->node(ix));
    std::shared_ptr<cdk::basic_type> etype = child->type();
    child->accept(this, lvl); // expression to print
    if (etype->name() == cdk::TYPE_INT)
    {
      _functions_to_declare.insert("printi");
      _pf.CALL("printi");
      _pf.TRASH(4); // trash int
    }
    else if (etype->name() == cdk::TYPE_STRING)
    {
      _functions_to_declare.insert("prints");
      _pf.CALL("prints");
      _pf.TRASH(4); // trash char pointer
    }
    else if (etype->name() == cdk::TYPE_DOUBLE)
    {
      _functions_to_declare.insert("printd");
      _pf.CALL("printd");
      _pf.TRASH(8); // trash char pointer
    }
    else
    {
      std::cerr << "cannot print expression of unknown type" << std::endl;
      return;
    }
  }

  if (node->newline())
  {
    _functions_to_declare.insert("println");
    _pf.CALL("println");
  }
}

void l22::postfix_writer::do_evaluation_node(l22::evaluation_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_evaluation_node(l22::evaluation_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl);
  if (node->argument()->is_typed(cdk::TYPE_INT) ||
      node->argument()->is_typed(cdk::TYPE_STRING) ||
      node->argument()->is_typed(cdk::TYPE_POINTER))
  {
    _pf.TRASH(4);
  }
  else if (node->argument()->is_typed(cdk::TYPE_DOUBLE))
  {
    _pf.TRASH(8);
  }
  else if (!node->argument()->is_typed(cdk::TYPE_VOID) &&
           !node->argument()->is_typed(cdk::TYPE_STRUCT))
  {
    std::cerr << "ERROR: Invalid type." << std::endl;
    exit(1);
  }
}

//---------------------------------------------------------------------------

void l22::postfix_writer::do_neg_node(cdk::neg_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_neg_node(cdk::neg_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl);
  if (node->is_typed(cdk::TYPE_DOUBLE))
  {
    _pf.DNEG();
  }
  else if (node->is_typed(cdk::TYPE_INT))
  {
    _pf.NEG();
  }
}

void l22::postfix_writer::do_not_node(cdk::not_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_not_node(cdk::not_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;

  node->argument()->accept(this, lvl + 2);
  _pf.INT(0);
  _pf.EQ();
}

void l22::postfix_writer::do_identity_node(l22::identity_node *node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_identity_node(l22::identity_node *node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl); // determine the value
}

//---------------------------------------------------------------------------

void l22::postfix_writer::do_add_node(cdk::add_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_add_node(cdk::add_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  if (node->is_typed(cdk::TYPE_DOUBLE) && node->left()->is_typed(cdk::TYPE_INT))
  {
    _pf.I2D();
  }
  else if (node->is_typed(cdk::TYPE_POINTER) && node->left()->is_typed(cdk::TYPE_INT))
  {

    if (cdk::reference_type::cast(node->type())->referenced()->name() == cdk::TYPE_DOUBLE)
    {
      _pf.INT(3);
    }
    else
    {
      _pf.INT(2);
    }
    _pf.SHTL();
  }

  node->right()->accept(this, lvl);
  if (node->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT))
  {
    _pf.I2D();
  }
  else if (node->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_INT))
  {
    if (cdk::reference_type::cast(node->type())->referenced()->name() == cdk::TYPE_DOUBLE)
    {
      _pf.INT(3);
    }
    else
    {
      _pf.INT(2);
    }
    _pf.SHTL();
  }

  // check if both are pointer??

  if (node->is_typed(cdk::TYPE_DOUBLE))
  {
    _pf.DADD();
  }
  else
  {
    _pf.ADD();
  }
}

void l22::postfix_writer::do_sub_node(cdk::sub_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_sub_node(cdk::sub_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  if (node->is_typed(cdk::TYPE_DOUBLE) && node->left()->is_typed(cdk::TYPE_INT))
  {
    _pf.I2D();
  }
  else if (node->is_typed(cdk::TYPE_POINTER) && node->left()->is_typed(cdk::TYPE_INT))
  {
    if (cdk::reference_type::cast(node->type())->referenced()->name() == cdk::TYPE_DOUBLE)
    {
      _pf.INT(3);
    }
    else
    {
      _pf.INT(2);
    }
    _pf.SHTL();
  }

  node->right()->accept(this, lvl);
  if (node->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT))
  {
    _pf.I2D();
  }
  else if (node->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_INT))
  {
    if (cdk::reference_type::cast(node->type())->referenced()->name() == cdk::TYPE_DOUBLE)
    {
      _pf.INT(3);
    }
    else
    {
      _pf.INT(2);
    }
    _pf.SHTL();
  }
  if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_POINTER))
  {
    int lbl1;

    _pf.SUB();
    _pf.INT(cdk::reference_type::cast(node->left()->type())->referenced()->size());
    _pf.DIV();
    _pf.DUP32();
    _pf.INT(0);
    _pf.LT();
    _pf.JZ(mklbl(lbl1 = ++_lbl));
    _pf.NEG();
    _pf.ALIGN();
    _pf.LABEL(mklbl(lbl1));
  }
  else
  {
    if (node->is_typed(cdk::TYPE_DOUBLE))
    {
      _pf.DSUB();
    }
    else
    {
      _pf.SUB();
    }
  }
}

void l22::postfix_writer::do_mul_node(cdk::mul_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_mul_node(cdk::mul_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  if (node->is_typed(cdk::TYPE_DOUBLE) && node->left()->is_typed(cdk::TYPE_INT))
  {
    _pf.I2D();
  }

  node->right()->accept(this, lvl);
  if (node->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT))
  {
    _pf.I2D();
  }

  if (node->is_typed(cdk::TYPE_DOUBLE))
  {
    _pf.DMUL();
  }
  else
  {
    _pf.MUL();
  }
}

void l22::postfix_writer::do_div_node(cdk::div_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_div_node(cdk::div_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  if (node->is_typed(cdk::TYPE_DOUBLE) && node->left()->is_typed(cdk::TYPE_INT))
  {
    _pf.I2D();
  }

  node->right()->accept(this, lvl);
  if (node->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT))
  {
    _pf.I2D();
  }

  if (node->is_typed(cdk::TYPE_DOUBLE))
  {
    _pf.DDIV();
  }
  else
  {
    _pf.DIV();
  }
}

void l22::postfix_writer::do_mod_node(cdk::mod_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_mod_node(cdk::mod_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  node->right()->accept(this, lvl);
  _pf.MOD();
}

//---------------------------------------------------------------------------

void l22::postfix_writer::do_sizeof_node(l22::sizeof_node *node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_sizeof_node(l22::sizeof_node *node, int lvl)" << std::endl;
  cdk::typed_node *typed = dynamic_cast<cdk::typed_node *>(node->expression());
  _pf.INT(typed->type()->size());
  _pf.ADD();
}

//---------------------------------------------------------------------------

void l22::postfix_writer::do_ge_node(cdk::ge_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_ge_node(cdk::ge_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  if (node->left()->type()->name() == cdk::TYPE_INT && node->right()->type()->name() == cdk::TYPE_DOUBLE)
    _pf.I2D();

  node->right()->accept(this, lvl);
  if (node->right()->type()->name() == cdk::TYPE_INT && node->right()->type()->name() == cdk::TYPE_DOUBLE)
    _pf.I2D();

  if (node->left()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_DOUBLE))
  {
    _pf.DCMP();
    _pf.INT(0);
  }

  _pf.GE();
}

void l22::postfix_writer::do_gt_node(cdk::gt_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_gt_node(cdk::gt_node *const node, int lvl)" << std::endl;
  // ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  if (node->left()->type()->name() == cdk::TYPE_INT && node->right()->type()->name() == cdk::TYPE_DOUBLE)
    _pf.I2D();

  node->right()->accept(this, lvl);
  if (node->right()->type()->name() == cdk::TYPE_INT && node->right()->type()->name() == cdk::TYPE_DOUBLE)
    _pf.I2D();

  if (node->left()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_DOUBLE))
  {
    _pf.DCMP();
    _pf.INT(0);
  }

  _pf.GT();
}

void l22::postfix_writer::do_le_node(cdk::le_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_le_node(cdk::le_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  if (node->left()->type()->name() == cdk::TYPE_INT && node->right()->type()->name() == cdk::TYPE_DOUBLE)
    _pf.I2D();

  node->right()->accept(this, lvl);
  if (node->right()->type()->name() == cdk::TYPE_INT && node->right()->type()->name() == cdk::TYPE_DOUBLE)
    _pf.I2D();

  if (node->left()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_DOUBLE))
  {
    _pf.DCMP();
    _pf.INT(0);
  }

  _pf.LE();
}

void l22::postfix_writer::do_lt_node(cdk::lt_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_lt_node(cdk::lt_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  if (node->left()->type()->name() == cdk::TYPE_INT && node->right()->type()->name() == cdk::TYPE_DOUBLE)
    _pf.I2D();

  node->right()->accept(this, lvl);
  if (node->right()->type()->name() == cdk::TYPE_INT && node->right()->type()->name() == cdk::TYPE_DOUBLE)
    _pf.I2D();

  if (node->left()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_DOUBLE))
  {
    _pf.DCMP();
    _pf.INT(0);
  }

  _pf.LT();
}

//---------------------------------------------------------------------------

void l22::postfix_writer::do_and_node(cdk::and_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_and_node(cdk::and_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  int lbl = ++_lbl;
  node->left()->accept(this, lvl + 2);
  _pf.DUP32();
  _pf.JZ(mklbl(lbl));
  node->right()->accept(this, lvl + 2);
  _pf.AND();
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl));
}

void l22::postfix_writer::do_or_node(cdk::or_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_or_node(cdk::or_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  int lbl = ++_lbl;
  node->left()->accept(this, lvl + 2);
  _pf.DUP32();
  _pf.JZ(mklbl(lbl));
  node->right()->accept(this, lvl + 2);
  _pf.OR();
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl));
}

//---------------------------------------------------------------------------

void l22::postfix_writer::do_eq_node(cdk::eq_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_eq_node(cdk::eq_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  if (node->left()->type()->name() == cdk::TYPE_INT && node->right()->type()->name() == cdk::TYPE_DOUBLE)
    _pf.I2D();

  node->right()->accept(this, lvl);
  if (node->right()->type()->name() == cdk::TYPE_INT && node->right()->type()->name() == cdk::TYPE_DOUBLE)
    _pf.I2D();

  if (node->left()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_DOUBLE))
  {
    _pf.DCMP();
    _pf.INT(0);
  }

  _pf.EQ();
}

void l22::postfix_writer::do_ne_node(cdk::ne_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_ne_node(cdk::ne_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  if (node->left()->type()->name() == cdk::TYPE_INT && node->right()->type()->name() == cdk::TYPE_DOUBLE)
    _pf.I2D();

  node->right()->accept(this, lvl);
  if (node->right()->type()->name() == cdk::TYPE_INT && node->right()->type()->name() == cdk::TYPE_DOUBLE)
    _pf.I2D();

  if (node->left()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_DOUBLE))
  {
    _pf.DCMP();
    _pf.INT(0);
  }

  _pf.NE();
}

//---------------------------------------------------------------------------

void l22::postfix_writer::do_address_of_node(l22::address_of_node *node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_address_of_node(l22::address_of_node *node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS
  node->lvalue()->accept(this, lvl);
}

void l22::postfix_writer::do_stack_alloc_node(l22::stack_alloc_node *node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_stack_alloc_node(l22::stack_alloc_node *node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl);
  if (cdk::reference_type::cast(node->type())->referenced()->name() == cdk::TYPE_DOUBLE)
    _pf.INT(3);
  else
    _pf.INT(2);

  _pf.SHTL();
  _pf.ALLOC();
  _pf.SP();
}

void l22::postfix_writer::do_nullptr_node(l22::nullptr_node *node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_nullptr_node(l22::nullptr_node *node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  if (_inFunctionBody)
  {
    _pf.INT(0);
  }
  else
  {
    _pf.SINT(0);
  }
}

//---------------------------------------------------------------------------

void l22::postfix_writer::do_while_node(l22::while_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_while_node(l22::while_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  int lbl1 = ++_lbl;
  _cond.push(lbl1);
  int lbl2 = ++_lbl;
  _end.push(lbl2);

  _pf.LABEL(mklbl(lbl1));
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(lbl2));
  node->block()->accept(this, lvl + 2);
  _pf.JMP(mklbl(lbl1));
  _pf.LABEL(mklbl(lbl2));

  _cond.pop();
  _end.pop();
}

void l22::postfix_writer::do_again_node(l22::again_node *node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_again_node(l22::again_node *node, int lvl)" << std::endl;
  if (_cond.size() > 0)
    _pf.JMP(mklbl(_cond.top()));
  else
    std::cerr << "Continue instruction can only be used inside a for cycle." << std::endl;
}

void l22::postfix_writer::do_stop_node(l22::stop_node *node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_stop_node(l22::stop_node *node, int lvl)" << std::endl;
  if (_end.size() > 0)
    _pf.JMP(mklbl(_end.top()));
  else
    std::cerr << "Break instruction can only be used inside a for cycle." << std::endl;
}

//---------------------------------------------------------------------------

void l22::postfix_writer::do_if_node(l22::if_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_if_node(l22::if_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  int lbl1;
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(lbl1 = ++_lbl));
  node->block()->accept(this, lvl + 2);
  _pf.LABEL(mklbl(lbl1));
}

void l22::postfix_writer::do_if_else_node(l22::if_else_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_if_else_node(l22::if_else_node *const node, int lvl)" << std::endl;
  ASSERT_SAFE_EXPRESSIONS;
  int lbl1, lbl2;
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(lbl1 = ++_lbl));
  node->thenblock()->accept(this, lvl + 2);
  _pf.JMP(mklbl(lbl2 = ++_lbl));
  _pf.LABEL(mklbl(lbl1));
  node->elseblock()->accept(this, lvl + 2);
  _pf.LABEL(mklbl(lbl1 = lbl2));
}

//---------------------------------------------------------------------------

void l22::postfix_writer::do_lambda_node(l22::lambda_node *node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_lambda_node(l22::lambda_node *node, int lvl)" << std::endl;
}

void l22::postfix_writer::do_function_call_node(l22::function_call_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_function_call_node(l22::function_call_node *const node, int lvl)" << std::endl;
}

void l22::postfix_writer::do_block_node(l22::block_node *const node, int lvl)
{
  std::cout << "void l22::postfix_writer::do_block_node(l22::block_node *const node, int lvl)" << std::endl;
  _symtab.push();
  if (node->declarations())
    node->declarations()->accept(this, lvl);

  if (node->instructions())
    node->instructions()->accept(this, lvl);
  _symtab.pop();
}
