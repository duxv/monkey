#include "../header/evaluator.h"
#include <iostream>

namespace monkey {

/*
 * builtin
 */

Object* print(std::vector<Object*>& objs) {
  for (auto obj : objs) {
    if(obj->Type() == ERROR_OBJ)
      return obj;
    std::cout << obj->Inspect() << " ";
  }
  std::cout << std::endl;
  return __NULL;
}

std::unordered_map<std::string, Builtin*> builtin({
  {"print", new Builtin(*print)}
});

bool isTruthy(Object* condition) {
  if(condition == __TRUE) {
    return true;
  } else if(condition == __FALSE) {
    return false;
  } else if (condition == __NULL) {
    return false;
  } else if (condition->Type() == INTEGER_OBJ && ((Integer*)condition)->value == 0) {
    return false;
  } else {
    return true;
  }
}

bool isError(Object* o) {
  if(o != nullptr) {
    return o->Type() == ERROR_OBJ;
  }
  return false;
}

Object* Evaluator::evalStatements(std::vector<Statement*> statements, Environment* env) {
  Object* result;
  for(auto stmt : statements) {
    result = Eval(stmt, env);
    gc.Mark(result);  // mark the intermediate result
    if (result->Type() == RETURN_VALUE_OBJ) {
      gc.Mark(((ReturnValue*)result)->value);
    }
    gcCounter++;
    if(gcCounter == 100) {
      gc.Mark(env);
      gc.Sweep();
      gcCounter = 0;
    }
    if (result->Type() == RETURN_VALUE_OBJ || result->Type() == ERROR_OBJ)
      return result;
  }
  return result;
}

Object* Evaluator::evalBangOperatorExpression(Object* right) {
  if (right == __TRUE) {
    return __FALSE;
  } else if (right == __FALSE) {
    return __TRUE;
  } else if (right == __NULL) {
    return __TRUE;
  } else if (right->Type() == INTEGER_OBJ) {
    if(((Integer*)right)->value == 0) {
      return __TRUE;
    } else {
      return __FALSE;
    }
  } else {
    return __FALSE;
  }
}

Object* Evaluator::evalMinusPrefixExpression(Object* right) {
  if(right->Type() == INTEGER_OBJ) {
    Integer* i = new Integer(-((Integer*)right)->value);
    return i;
  }
  return new Error("unknown operator: -" +  right->Type());
}

Object* Evaluator::evalPrefixExpression(std::string op, Object* right) {
  if(op == "!") {
    return evalBangOperatorExpression(right);
  } else if (op == "-") {
    return evalMinusPrefixExpression(right);
  } else {
    return __NULL;
  }
}

Object* Evaluator::evalIntegerInfixExpression(std::string op, Object* left, Object* right) {
  int leftVal = ((Integer*)left)->value;
  int rightVal = ((Integer*)right)->value;
    Object* res;
  if(op == "+") {
    res = new Integer(leftVal + rightVal);
  }
  else if(op == "-") {
    res = new Integer(leftVal - rightVal);
  }
  else if(op == "*") {
    res = new Integer(leftVal * rightVal);
  }
  else if(op == "/" && rightVal != 0) {
    res = new Integer(leftVal / rightVal);
  }
  else if(op == "%" && rightVal != 0) {
    res = new Integer(leftVal % rightVal);
  }
  else if(op == "==") {
    return leftVal == rightVal ? __TRUE : __FALSE;
  }
  else if(op == "!=") {
    return leftVal != rightVal ? __TRUE : __FALSE;
  }
  else if(op == ">") {
    return leftVal > rightVal ? __TRUE : __FALSE;
  }
  else if(op == "<") {
    return leftVal < rightVal ? __TRUE : __FALSE;
  }
  else if(op == ">=") {
    return leftVal >= rightVal ? __TRUE : __FALSE;
  }
  else if(op == "<=") {
    return leftVal <= rightVal ? __TRUE : __FALSE;
  }
  else {
    return new Error("unknown operator: " + left->Type() + " " + op + " " +  right->Type());
  }
  gc.Add(res);
  return res;
  }

Object* Evaluator::evalStringInfixExpression(std::string op, Object* left, Object* right) {
  std::string leftVal = ((String*)left)->value;
  std::string rightVal = ((String*)right)->value;
  Object* res;
  if(op == "+") {
    res = new String(leftVal + rightVal);
  } else {
    return new Error("unknown operator: " + left->Type() + " " + op + " " +  right->Type());
  }
  gc.Add(res);
  return res;
}

Object* Evaluator::evalInfixExpression(std::string op, Object* left, Object* right) {
  if (left->Type() == INTEGER_OBJ && right->Type() == INTEGER_OBJ) {
    return evalIntegerInfixExpression(op, left, right);
  } else if (left->Type() == STRING_OBJ && right->Type() == STRING_OBJ) {
    return evalStringInfixExpression(op, left, right);
  } else if(left == __NULL || right == __NULL) {
    return __NULL;
  } else if (left->Type() != right->Type()) {
    return new Error(
        "type mismatch: " + left->Type() + " " + op + " " +  right->Type());
  } else if(op == "==") {
    return left == right ? __TRUE : __FALSE;
  } else if(op == "!=") {
    return left != right ? __TRUE : __FALSE;
  } else {
    return new Error(
        "unknown operator: " + left->Type() + " " + op + " " +  right->Type());
  }
}

Environment* Evaluator::extendedFunctionEnv(Function* fn, std::vector<Object*>& args, Environment* outer) {
  Environment* env = outer->NewEnclosedEnvironment();
  for(int i=0; i<args.size(); i++) {
    env->Set(fn->parameters[i]->value, args[i]);
  }
  return env;
}

Object* Evaluator::evalCallExpression(Object* fn, std::vector<Object*>& args, Environment* env) {
  if(fn->Type() != FUNCTION_OBJ && fn->Type() != BUILTIN_OBJ) {
    return new Error("not a function: " + fn->Type());
  }
  if(fn->Type() == BUILTIN_OBJ) {
    return ((Builtin*)fn)->function(args);
  }
  if(((Function*)fn)->parameters.size() != args.size()) {
    return new Error("argument length(" + std::to_string(args.size()) +
        ") not equal to parameter length (" 
        + std::to_string(((Function*)fn)->parameters.size()) + ")");
  }
  Environment* extendedEnv = extendedFunctionEnv((Function*)fn, args, env);
  Object* evaluated = Eval(((Function*)fn)->body, extendedEnv);
  delete extendedEnv;
  if(evaluated->Type() == RETURN_VALUE_OBJ) {
    return ((ReturnValue*)evaluated)->value;
  }
  return evaluated;
}

Object* Evaluator::evalArrayIndexExpression(Array* array, Integer* index) {
  int i = index->value;
  auto arr = array->elements;
  try {
    return arr[i];
  } catch (const std::out_of_range e) {
    return new Error("index " + index->Inspect() + " out of range");
  }
}

Object* Evaluator::evalStringIndexExpression(String* array, Integer* index) {
  int i = index->value;
  auto arr = array->value;
  try {
    return new String(std::string(1, arr[i]));
  } catch (const std::out_of_range e) {
    return new Error("index " + index->Inspect() + " out of range");
  }
}

Object* Evaluator::evalIndexExpression(Object* array, Object* index, Environment* env) {
  if (array->Type() == ARRAY_OBJ && index->Type() == INTEGER_OBJ)
    return evalArrayIndexExpression((Array*)array, (Integer*)index);
  if (array->Type() == STRING_OBJ && index->Type() == INTEGER_OBJ)
    return evalStringIndexExpression((String*)array, (Integer*)index);
  else {
    return new Error("index operator not supported: " + array->Type());
  }
}

Object* Evaluator::evalIdentifier(std::string name, Environment* env) {
  Object* obj =  env->Get(name);
  if (obj->Type() == ERROR_OBJ && builtin.find(name) != builtin.end()) {
    return builtin[name];
  }
  return obj;
}

Object* Evaluator::evalProgram(Program* program, Environment* env) {
  Object* o = evalStatements(program->statements, env);
  if (o->Type() == RETURN_VALUE_OBJ) {  // unwrap return value
    return ((ReturnValue*)o)->value;
  }
  return o;
}

Object* Evaluator::Eval(Node* node, Environment* env) {
  NodeType type = node->Type();
  if (type == NodeProgram) {
    return evalProgram((Program*)node, env);
  } else if (type == ExprInt) {
    Integer* i = new Integer(((IntegerLiteral*)node)->value);
    gc.Add(i);
    return i;
  } else if (type == ExprBoolLit) {
    return ((BooleanLiteral*)node)->value ? __TRUE : __FALSE;
  } else if (type == ExprStringLit) {
    String* s = new String(((StringLiteral*)node)->value);
    gc.Add(s);
    return s;
  } else if (type == ExprIdent) {
    return evalIdentifier(((Identifier*)node)->value, env);
  } else if (type == ExprFuncLit) {
    Function* f =  new Function(((FunctionLiteral*)node)->parameters, ((FunctionLiteral*)node)->body);
    gc.Add(f);
    return f;
  } else if (type == ExprCall) {
    Object* function = Eval(((CallExpression*)node)->function, env);
    if(isError(function))
      return function;
    
    std::vector<Object*> args;
    for(auto* argument : ((CallExpression*)node)->arguments) {  // for convenience, using pass by value
      Object* arg = Eval(argument, env);
      if(isError(arg))
      return arg;
      args.push_back(arg);
    }
    return evalCallExpression(function, args, env);
  } else if (type == ExprIndex) {
    Object* array = Eval(((IndexExpression*)node)->array, env);
    if(isError(array)) {
      return array;
    }
    env->Set(std::to_string((intptr_t)array), array);
    Object* index = Eval(((IndexExpression*)node)->index, env);
    env->store.erase(std::to_string((intptr_t)array));
    if(isError(index)) {
      return index;
    }
    return evalIndexExpression(array, index, env);
  } else if (type == ExprArrayLit) {
    std::vector<Object*> elems;
    for(auto* element : ((ArrayLiteral*)node)->elements) {  // for convenience, using pass by value
      Object* elem = Eval(element, env);
      if(isError(elem))
      return elem;
      elems.push_back(elem);
    }
    return new Array(elems);
  } else if (type == ExprPrefix) {
    Object* right = Eval(((PrefixExpression*)node)->right, env);
    if (isError(right))
      return right;
    return evalPrefixExpression(((PrefixExpression*)node)->op, right);
  } else if (type == ExprInfix) {
    Object* left = Eval(((InfixExpression*)node)->left, env);
    if (isError(left))
      return left;
    // save tmp data
    // TODO: find a better way
    env->Set(std::to_string((intptr_t)left), left);
    Object* right = Eval(((InfixExpression*)node)->right, env);
    env->store.erase(std::to_string((intptr_t)left));
    if (isError(right))
      return right;
    return evalInfixExpression(((InfixExpression*)node)->op, left, right);
  } else if (type == ExprIf) {
    Object* condition = Eval(((IfExpression*)node)->condition, env);
    if(isError(condition))
      return condition;
    if(isTruthy(condition)) {
      return Eval(((IfExpression*)node)->consequence, env);
    } else if(((IfExpression*)node)->alternative != nullptr) {
      return Eval(((IfExpression*)node)->alternative, env);
    } else {
      return __NULL;
    }
  } else if (type == ExprWhile) {
    while (true) {
      Object* condition = Eval(((WhileExpression*)node)->condition, env);
      if(isError(condition))
      return condition;
      if(!isTruthy(condition))
      return __NULL;
      Object* result = Eval(((WhileExpression*)node)->consequence, env);
      if (result->Type() == ERROR_OBJ || result->Type() == RETURN_VALUE_OBJ)
      return result;
    }
  } else if (type == ExprStmt) {
    return Eval(((ExpressionStatement*)node)->expression, env);
  } else if (type == BlockStmt) {
    return evalStatements(((BlockStatement*)node)->statements, env);
  } else if (type == ReturnStmt) {
    Object* val = Eval(((ReturnStatement*)node)->returnValue, env);
    if(isError(val))
      return val;
    return new ReturnValue(val);
  } else if (type == LetStmt) {
    Object* val = Eval(((LetStatement*)node)->value, env);
    if(isError(val))
      return val;
    return env->Set(((LetStatement*)node)->name.value, val);
  } else if (type == RefStmt) {
    Object* val = Eval(((RefStatement*)node)->value, env);
    if(isError(val))
      return val;
    return env->RefSet(((LetStatement*)node)->name.value, val);
  } else {
    return __NULL;
  }
}

}  // namespace monkey