/**
   @file semantics.cpp
   @brief Implementation for semantic analysis-related functionality of cremacc checking
   @copyright 2014 Assured Information Security, Inc.
   @author Jacob Torrey <torreyj@ainfosec.com>

   This file contains the implementations for all functionality related to semantic analysis.
   This includes the SemanticContext implementation and the typing functions for the AST functions.
 */
#include "semantics.h"
#include "ast.h"
#include "parser.h"
#include "types.h"
#include <typeinfo>

/**
 * The root SemanticContext object to use when performing semantic analysis */
SemanticContext rootCtx;

/** 
    Creates a new scope for variable declarations

    @param type The return type of the scope (0 if void)
 */
void SemanticContext::newScope(int type)
{
  vars.push_back(new VariableList());
  currType.push_back(type);
  currScope++;
}

/**
   Deletes the most recent scope
*/
void SemanticContext::delScope()
{
  vars.pop_back();
  currType.pop_back();
  currScope--;
}

/**
   Registers the variable into the current scope

   @param var Pointer to the NVariableDeclaration to add to the current scope
   @return true if the variable was added, false if it is a duplicate
*/
bool SemanticContext::registerVar(NVariableDeclaration * var)
{
  // Search through current scope for variable duplication
  for (int j = 0; j < (vars[currScope])->size(); j++)
    {
      if (var->ident == vars[currScope]->at(j)->ident)
	return false;
    }

  vars[currScope]->push_back(var);
  return true;
}

/**
   Registers the function into the global scope

   @param func Pointer to the NFunctionDeclaration to add to the global scope
   @return true if the function was added, false if it is a duplicate
*/
bool SemanticContext::registerFunc(NFunctionDeclaration * func)
{
  // Search through for duplicate function duplication
  for (int j = 0; j < funcs.size(); j++)
    {
      if (func->ident == funcs[j]->ident)
	return false;
    }

  funcs.push_back(func);
  return true;
}

/**
   Registers the structure into the global scope

   @param s Pointer to the NStructureDeclaration to add to the global scope
   @return true if the structure was added, false if it is a duplicate
*/
bool SemanticContext::registerStruct(NStructureDeclaration * s)
{
  // Search through for duplicate struct duplication
  for (int j = 0; j < structs.size(); j++)
    {
      if (s->ident == structs[j]->ident)
	return false;
    }

  structs.push_back(s);
  return true;
}

/**
  Searches the local, then parent scopes for a variable declaration

  @param ident NIdentifier to search for in the stack of scopes
  @return Pointer to NVariableDeclaration of the referenced variable or NULL if it cannot be found
 */
NVariableDeclaration * SemanticContext::searchVars(NIdentifier & ident) 
{
  // Search through stacks in reverse order
  for (int i = vars.size() - 1; i >= 0; i--)
    {
      // Search through current scope for variable
      for (int j = 0; j < (vars[i])->size(); j++)
	{
	  if (ident == vars[i]->at(j)->ident)
	    return vars[i]->at(j);
	}
    }

  return NULL;
}

/**
  Searches for a function declaration

  @param ident NIdentifier to search for in the global function scope
  @return Pointer to NFunctionDeclaration of the referenced function or NULL if it cannot be found
*/
NFunctionDeclaration * SemanticContext::searchFuncs(NIdentifier & ident) 
{
  for (int i = 0; i < funcs.size(); i++)
    {
      if (ident == funcs[i]->ident)
	return funcs[i];
    }

  return NULL;
}


/**
  Searches for a structure declaration

  @param ident NIdentifier to search for in the global structure scope
  @return Pointer to NStructureDeclaration of the referenced structure or NULL if it cannot be found
*/
NStructureDeclaration * SemanticContext::searchStructs(NIdentifier & ident) 
{
  for (int i = 0; i < structs.size(); i++)
    {
      if (ident == structs[i]->ident)
	return structs[i];
    }

  return NULL;
}

/**
   Iterates over the vector and returns 'false' if any of
   semanticAnalysis elements are false.

   @param ctx Pointer to the SemanticContext on which to perform the checks
   @return true if the block passes semantic analysis, false otherwise
*/
bool NBlock::semanticAnalysis(SemanticContext * ctx)
{
  // Points to the last element in the vector<int> currType.
  ctx->newScope(ctx->currType.back());
  for (int i = 0; i < statements.size(); i++)
    {
      if (!((*(statements[i])).semanticAnalysis(ctx)))
	return false;
    }
  ctx->delScope();
  return true;
}

/**
   Iterates over the vector and returns 'true' if any of
   checkRecursion elements are true.

   @param ctx Pointer to SemanticContext on which to perform the checks
   @param func Pointer to NFunctionDeclaration of the parent function that is being checked
   @return true if there is a recursive call, false otherwise
*/
bool NBlock::checkRecursion(SemanticContext *ctx, NFunctionDeclaration * func)
{
  for (int i = 0; i < statements.size(); i++)
    {
      if (((*(statements[i])).checkRecursion(ctx, func)))
	return true;
    }
  return false;
}

bool NFunctionCall::checkRecursion(SemanticContext * ctx, NFunctionDeclaration * func)
{
  if (func->ident == ident)
    {
      return true;
    }
  
  return ctx->searchFuncs(ident)->body->checkRecursion(ctx, func);
}

bool NBinaryOperator::semanticAnalysis(SemanticContext * ctx)
{
  if (lhs.getType(ctx) != rhs.getType(ctx))
    {
      std::cout << "Binary operator type mismatch for op: " << op << std::endl;
      return false;
    }
  return true;
}

bool NAssignmentStatement::semanticAnalysis(SemanticContext * ctx)
{
  NVariableDeclaration *var = ctx->searchVars(ident);
  int type = (var->size == 1) ? var->type : var->type | 0xF0000000;
  if (!var)
    {
      std::cout << "Assignment to undefined variable " << ident << std::endl;
      return false;
    }

  if (type != expr.getType(ctx))
    {
      std::cout << "Type mismatch for assignment to " << ident << std::endl;
      return false;
    }
  return true;
}

bool NReturn::semanticAnalysis(SemanticContext * ctx)
{
  if (retExpr.getType(ctx) != ctx->currType.back())
    {
      std::cout << "Returning type " << retExpr.getType(ctx) << " when a " << ctx->currType.back() << " was expected" << std::endl;
      return false;
    }
  return true;
}

int NList::getType(SemanticContext * ctx) const
{
  if (value.size() == 0)
    return 0;
  int type = value[0]->getType(ctx);
  for (int i = 1; i < value.size(); i++)
    {
      if (value[i]->getType(ctx) != type)
	return 0;
    }

  return 0xF0000000 | type;
}

int NVariableAccess::getType(SemanticContext * ctx) const
{
  NVariableDeclaration *var = ctx->searchVars(ident);
  if (var)
    {
      return (var->size == 1) ? var->type : 0xF0000000 | var->type;
    }
  return 0;
}

int NFunctionCall::getType(SemanticContext * ctx) const 
{
  NFunctionDeclaration *func = ctx->searchFuncs(ident);
  if (func)
    {
      return func->listReturn ? 0xF0000000 | func->type : func->type;
    }
  return 0;
}

bool NFunctionCall::semanticAnalysis(SemanticContext * ctx)
{
  NFunctionDeclaration *func = ctx->searchFuncs(ident);
  if (func)
    {
      if (func->variables.size() != args.size())
	{
	  std::cout << "Call to " << ident << " with invalid number of arguments! " << func->variables.size() << " expected, " << args.size() << " provided" << std::endl;
	  return false;
	}
      for (int i = 0; i < args.size(); i++)
	{
	  if (args[i]->getType(ctx) != func->variables[i]->type)
	    {
	      std::cout << "Type mismatch when calling function: " << ident << std::endl;
	      return false;
	    }
	}
      return true;
    }
  std::cout << "Call to undefined function: " << ident << std::endl;
  return false;
}

bool NFunctionDeclaration::semanticAnalysis(SemanticContext * ctx)
{
  bool blockSA, blockRecur;
  ctx->newScope(listReturn ? type | 0xF0000000 : type);
  for (int i = 0; i < variables.size(); i++)
    {
      ctx->registerVar(variables[i]);
    }
  blockSA = body->semanticAnalysis(ctx);
  blockRecur = body->checkRecursion(ctx, this);
  if (blockRecur)
    {
      std::cout << "Recursive function call in " << ident << std::endl;
    }
  ctx->delScope();
  return (blockSA && !blockRecur);
}

bool NVariableDeclaration::semanticAnalysis(SemanticContext * ctx)
{
  int ltype = (size == 1) ? type : 0xF0000000 | type;
  if (!ctx->registerVar(this)) 
    {
      std::cout << "Duplicate var decl for " << ident << std::endl;
      // Variable collision
      return false;
    } 
  if (initializationExpression)
    {
      if (initializationExpression->getType(ctx) != ltype || !initializationExpression->semanticAnalysis(ctx))
	{
	  std::cout << "Type mismatch for " << ident << std::endl;
	  return false;
	}
    }
  return true;
}


