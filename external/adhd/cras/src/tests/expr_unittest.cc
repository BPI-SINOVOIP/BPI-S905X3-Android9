// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "cras_expr.h"

namespace {

TEST(ExprTest, UnparsibleExpression) {
  struct cras_expr_expression *expr;

  /* un-parsable input */
  expr = cras_expr_expression_parse("#");
  EXPECT_EQ(NULL, expr);

  expr = cras_expr_expression_parse(NULL);
  EXPECT_EQ(NULL, expr);

  cras_expr_expression_free(expr);
}

TEST(ExprTest, LiteralExpression) {
  struct cras_expr_expression *expr;
  struct cras_expr_value value = CRAS_EXPR_VALUE_INIT;
  struct cras_expr_env env = CRAS_EXPR_ENV_INIT;
  int integer = 0;

  /* parse integer literal */
  expr = cras_expr_expression_parse(" -2");
  EXPECT_EQ(EXPR_TYPE_LITERAL, expr->type);
  EXPECT_EQ(CRAS_EXPR_VALUE_TYPE_INT, expr->u.literal.type);
  EXPECT_EQ(-2, expr->u.literal.u.integer);

  /* evaluate integer literal */
  cras_expr_expression_eval(expr, &env, &value);
  EXPECT_EQ(CRAS_EXPR_VALUE_TYPE_INT, value.type);
  EXPECT_EQ(-2, value.u.integer);

  EXPECT_EQ(0, cras_expr_expression_eval_int(expr, &env, &integer));
  EXPECT_EQ(-2, integer);
  cras_expr_expression_free(expr);

  /* parse string literal */
  expr = cras_expr_expression_parse("\"hello\" ");
  EXPECT_EQ(EXPR_TYPE_LITERAL, expr->type);
  EXPECT_EQ(CRAS_EXPR_VALUE_TYPE_STRING, expr->u.literal.type);
  EXPECT_STREQ("hello", expr->u.literal.u.string);

  /* evaluate string literal */
  cras_expr_expression_eval(expr, &env, &value);
  ASSERT_EQ(CRAS_EXPR_VALUE_TYPE_STRING, value.type);
  EXPECT_STREQ("hello", value.u.string);

  cras_expr_value_free(&value);
  cras_expr_expression_free(expr);
  cras_expr_env_free(&env);
}

TEST(ExprTest, Variable) {
  struct cras_expr_expression *expr;
  struct cras_expr_value value = CRAS_EXPR_VALUE_INIT;
  struct cras_expr_env env = CRAS_EXPR_ENV_INIT;
  int integer = 0;

  /* parse variable */
  expr = cras_expr_expression_parse("a");
  ASSERT_EQ(EXPR_TYPE_VARIABLE, expr->type);
  EXPECT_STREQ("a", expr->u.variable);

  /* evaluate variable (undefined now) */
  cras_expr_expression_eval(expr, &env, &value);
  EXPECT_EQ(CRAS_EXPR_VALUE_TYPE_NONE, value.type);

  /* undefined variable */
  EXPECT_EQ(-1, cras_expr_expression_eval_int(expr, &env, &integer));

  /* now define a variable with integer value 2 */
  cras_expr_env_set_variable_integer(&env, "a", 2);

  /* re-evaluate the variable */
  EXPECT_EQ(0, cras_expr_expression_eval_int(expr, &env, &integer));
  EXPECT_EQ(2, integer);

  cras_expr_value_free(&value);
  cras_expr_expression_free(expr);
  cras_expr_env_free(&env);
}

TEST(ExprTest, Compound) {
  struct cras_expr_expression *expr, *expr1, *expr2;
  struct cras_expr_value value = CRAS_EXPR_VALUE_INIT;
  struct cras_expr_env env = CRAS_EXPR_ENV_INIT;

  /* parse empty compound expression */
  expr = cras_expr_expression_parse("()");
  ASSERT_EQ(EXPR_TYPE_COMPOUND, expr->type);
  EXPECT_EQ(0, ARRAY_COUNT(&expr->u.children));

  /* evaluate empty compound expression */
  cras_expr_expression_eval(expr, &env, &value);
  EXPECT_EQ(CRAS_EXPR_VALUE_TYPE_NONE, value.type);
  cras_expr_expression_free(expr);

  /* parse non-empty compound expression */
  expr = cras_expr_expression_parse("(foo bar)");
  ASSERT_EQ(EXPR_TYPE_COMPOUND, expr->type);
  EXPECT_EQ(2, ARRAY_COUNT(&expr->u.children));

  /* evaluate non-empty compound expression */
  cras_expr_expression_eval(expr, &env, &value);
  EXPECT_EQ(CRAS_EXPR_VALUE_TYPE_NONE, value.type);
  cras_expr_expression_free(expr);

  /* parse nested compound expression */
  expr = cras_expr_expression_parse("((foo 3)bar )");
  ASSERT_EQ(EXPR_TYPE_COMPOUND, expr->type);
  ASSERT_EQ(2, ARRAY_COUNT(&expr->u.children));

  expr1 = *ARRAY_ELEMENT(&expr->u.children, 0);
  ASSERT_EQ(EXPR_TYPE_COMPOUND, expr1->type);
  ASSERT_EQ(2, ARRAY_COUNT(&expr1->u.children));

  expr2 = *ARRAY_ELEMENT(&expr1->u.children, 1);
  ASSERT_EQ(EXPR_TYPE_LITERAL, expr2->type);
  EXPECT_EQ(CRAS_EXPR_VALUE_TYPE_INT, expr2->u.literal.type);
  EXPECT_EQ(3, expr2->u.literal.u.integer);

  /* evaluate nested compound expression */
  cras_expr_expression_eval(expr, &env, &value);
  EXPECT_EQ(CRAS_EXPR_VALUE_TYPE_NONE, value.type);
  cras_expr_expression_free(expr);

  cras_expr_value_free(&value);
  cras_expr_env_free(&env);
}

TEST(ExprTest, Environment) {
  struct cras_expr_expression *expr;
  struct cras_expr_value value = CRAS_EXPR_VALUE_INIT;
  struct cras_expr_env env1 = CRAS_EXPR_ENV_INIT;
  struct cras_expr_env env2 = CRAS_EXPR_ENV_INIT;
  int integer = 0;
  char boolean = 0;

  /* parse variable */
  expr = cras_expr_expression_parse("baz");

  /* put baz=4 into env1 */
  cras_expr_env_set_variable_integer(&env1, "baz", 4);

  /* evaluate expr against env1 and env2 */
  EXPECT_EQ(0, cras_expr_expression_eval_int(expr, &env1, &integer));
  EXPECT_EQ(4, integer);
  EXPECT_EQ(-1, cras_expr_expression_eval_int(expr, &env2, &integer));

  /* put baz=5 into env2 */
  cras_expr_env_set_variable_integer(&env2, "baz", 5);

  /* evaluate again */
  EXPECT_EQ(0, cras_expr_expression_eval_int(expr, &env1, &integer));
  EXPECT_EQ(4, integer);
  EXPECT_EQ(0, cras_expr_expression_eval_int(expr, &env2, &integer));
  EXPECT_EQ(5, integer);

  /* an integer is not a boolean */
  EXPECT_EQ(-1, cras_expr_expression_eval_boolean(expr, &env2, &boolean));

  cras_expr_value_free(&value);
  cras_expr_expression_free(expr);
  cras_expr_env_free(&env1);
  cras_expr_env_free(&env2);
}

static void expect_int(int expected, const char *str, struct cras_expr_env *env)
{
  struct cras_expr_expression *expr;
  struct cras_expr_value value = CRAS_EXPR_VALUE_INIT;

  expr = cras_expr_expression_parse(str);
  cras_expr_expression_eval(expr, env, &value);
  EXPECT_EQ(CRAS_EXPR_VALUE_TYPE_INT, value.type);
  EXPECT_EQ(expected, value.u.integer);
  cras_expr_expression_free(expr);
}

static void expect_boolean(char expected, const char *str,
                           struct cras_expr_env *env)
{
  struct cras_expr_expression *expr;
  struct cras_expr_value value = CRAS_EXPR_VALUE_INIT;

  expr = cras_expr_expression_parse(str);
  cras_expr_expression_eval(expr, env, &value);
  EXPECT_EQ(CRAS_EXPR_VALUE_TYPE_BOOLEAN, value.type);
  EXPECT_EQ(expected, value.u.boolean);
  cras_expr_expression_free(expr);
}

TEST(ExprTest, Builtin) {
  struct cras_expr_expression *expr;
  struct cras_expr_value value = CRAS_EXPR_VALUE_INIT;
  struct cras_expr_env env = CRAS_EXPR_ENV_INIT;

  cras_expr_env_install_builtins(&env);

  /* parse variable */
  expr = cras_expr_expression_parse("or");
  cras_expr_expression_eval(expr, &env, &value);
  EXPECT_EQ(CRAS_EXPR_VALUE_TYPE_FUNCTION, value.type);
  cras_expr_expression_free(expr);

  /* test builtin functions */
  expect_boolean(1, "(and)", &env);
  expect_boolean(1, "(and #t)", &env);
  expect_boolean(1, "(and #t #t)", &env);
  expect_int(3, "(and 1 2 3)", &env);
  expect_boolean(0, "(and #f 4)", &env);
  expect_boolean(0, "(or)", &env);
  expect_boolean(1, "(or #t)", &env);
  expect_boolean(0, "(or #f #f)", &env);
  expect_int(2, "(or #f #f 2)", &env);
  expect_int(3, "(or #f (or 3))", &env);
  expect_boolean(0, "(equal? \"hello\" 3)", &env);
  expect_boolean(1, "(equal? \"hello\" \"hello\")", &env);

  /* a more complex example a="hello" b="world"*/
  expr = cras_expr_expression_parse("(or (equal? \"test\" a) b)");
  cras_expr_env_set_variable_string(&env, "a", "hello");
  cras_expr_env_set_variable_string(&env, "b", "world");

  cras_expr_expression_eval(expr, &env, &value);
  EXPECT_EQ(CRAS_EXPR_VALUE_TYPE_STRING, value.type);
  EXPECT_STREQ("world", value.u.string);
  cras_expr_expression_free(expr);

  cras_expr_value_free(&value);
  cras_expr_env_free(&env);
}

}  //  namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
