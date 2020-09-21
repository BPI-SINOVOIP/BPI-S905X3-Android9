/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package android.databinding;

import android.databinding.parser.BindingExpressionBaseVisitor;
import android.databinding.parser.BindingExpressionLexer;
import android.databinding.parser.BindingExpressionParser;
import android.databinding.parser.BindingExpressionParser.AndOrOpContext;
import android.databinding.parser.BindingExpressionParser.BinaryOpContext;
import android.databinding.parser.BindingExpressionParser.BindingSyntaxContext;
import android.databinding.parser.BindingExpressionParser.BitShiftOpContext;
import android.databinding.parser.BindingExpressionParser.ComparisonOpContext;
import android.databinding.parser.BindingExpressionParser.DefaultsContext;
import android.databinding.parser.BindingExpressionParser.DotOpContext;
import android.databinding.parser.BindingExpressionParser.ExpressionContext;
import android.databinding.parser.BindingExpressionParser.GroupingContext;
import android.databinding.parser.BindingExpressionParser.LiteralContext;
import android.databinding.parser.BindingExpressionParser.MathOpContext;
import android.databinding.parser.BindingExpressionParser.PrimaryContext;
import android.databinding.parser.BindingExpressionParser.PrimitiveTypeContext;
import android.databinding.parser.BindingExpressionParser.QuestionQuestionOpContext;
import android.databinding.parser.BindingExpressionParser.ResourceContext;
import android.databinding.parser.BindingExpressionParser.StringLiteralContext;
import android.databinding.parser.BindingExpressionParser.TernaryOpContext;
import android.databinding.parser.BindingExpressionParser.UnaryOpContext;

import org.antlr.v4.runtime.ANTLRInputStream;
import org.antlr.v4.runtime.CommonTokenStream;
import org.antlr.v4.runtime.Token;
import org.antlr.v4.runtime.misc.NotNull;
import org.antlr.v4.runtime.tree.TerminalNode;
import org.junit.Test;

import java.io.StringReader;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

public class BindingExpressionParserTest {

    @Test
    public void testSingleQuoteStringLiteral() throws Exception {
        String expr = "`test`";
        LiteralContext literal = parseLiteral(expr);
        assertNotNull(literal);
        StringLiteralContext stringLiteral = literal.stringLiteral();
        assertNotNull(stringLiteral);
        TerminalNode singleQuote = stringLiteral.SingleQuoteString();
        Token token = singleQuote.getSymbol();
        assertEquals("`test`", token.getText());
    }

    @Test
    public void testDoubleQuoteStringLiteral() throws Exception {
        String expr = "\"test\"";

        LiteralContext literal = parseLiteral(expr);
        StringLiteralContext stringLiteral = literal.stringLiteral();
        TerminalNode singleQuote = stringLiteral.DoubleQuoteString();
        Token token = singleQuote.getSymbol();
        assertEquals("\"test\"", token.getText());
    }

    @Test
    public void testSingleQuoteEscapeStringLiteral() throws Exception {
        String expr = "`\"t\\`est\"`";
        LiteralContext literal = parseLiteral(expr);
        StringLiteralContext stringLiteral = literal.stringLiteral();
        TerminalNode singleQuote = stringLiteral.SingleQuoteString();
        Token token = singleQuote.getSymbol();
        assertEquals("`\"t\\`est\"`", token.getText());
    }

    @Test
    public void testCharLiteral() throws Exception {
        LiteralContext literal = parseLiteral("'c'");
        assertEquals("'c'", literal.getText());
        literal = parseLiteral("'\\u0054'");
        assertEquals("'\\u0054'", literal.getText());
        literal = parseLiteral("'\\''");
        assertEquals("'\\''", literal.getText());
    }

    @Test
    public void testIntLiterals() throws Exception {
        compareIntLiteral("123");
        compareIntLiteral("123l");
        compareIntLiteral("1_2_3l");
        compareIntLiteral("123L");
        compareIntLiteral("0xdeadbeef");
        compareIntLiteral("0xdeadbeefl");
        compareIntLiteral("0Xdeadbeef");
        compareIntLiteral("0xdead_beefl");
        compareIntLiteral("0xdead_beefL");
        compareIntLiteral("01234567");
        compareIntLiteral("01234567L");
        compareIntLiteral("01234567l");
        compareIntLiteral("0123_45_67l");
        compareIntLiteral("0b0101");
        compareIntLiteral("0b0101_0101");
        compareIntLiteral("0B0101_0101");
        compareIntLiteral("0B0101_0101L");
        compareIntLiteral("0B0101_0101l");
    }

    @Test
    public void testFloatLiterals() throws Exception {
        compareFloatLiteral("0.12345");
        compareFloatLiteral("0.12345f");
        compareFloatLiteral("0.12345F");
        compareFloatLiteral("132450.12345F");
        compareFloatLiteral("132450.12345");
        compareFloatLiteral("132450e123");
        compareFloatLiteral("132450.4e123");
    }

    @Test
    public void testBoolLiterals() throws Exception {
        compareBoolLiteral("true");
        compareBoolLiteral("false");
    }

    @Test
    public void testNullLiteral() throws Exception {
        LiteralContext literal = parseLiteral("null");
        String token = literal.getText();
        assertEquals("null", token);
    }

    @Test
    public void testPrimitiveClassExtraction() throws Exception {
        PrimaryContext primary = parsePrimary("int.class");
        PrimitiveTypeContext type = primary.classExtraction().type().primitiveType();
        assertEquals("int", type.getText());
    }

    @Test
    public void testIdentifier() throws Exception {
        PrimaryContext primary = parsePrimary("abcdEfg");
        assertEquals("abcdEfg", primary.identifier().getText());
    }

    @Test
    public void testUnaryOperators() throws Exception {
        compareUnaryOperators("+");
        compareUnaryOperators("-");
        compareUnaryOperators("!");
        compareUnaryOperators("~");
    }

    @Test
    public void testMathOperators() throws Exception {
        compareMathOperators("+");
        compareMathOperators("-");
        compareMathOperators("*");
        compareMathOperators("/");
        compareMathOperators("%");
    }

    @Test
    public void testBitShiftOperators() throws Exception {
        compareBitShiftOperators(">>>");
        compareBitShiftOperators("<<");
        compareBitShiftOperators(">>");
    }

    @Test
    public void testComparisonShiftOperators() throws Exception {
        compareComparisonOperators("<");
        compareComparisonOperators(">");
        compareComparisonOperators("<=");
        compareComparisonOperators(">=");
        compareComparisonOperators("==");
        compareComparisonOperators("!=");
    }

    @Test
    public void testAndOrOperators() throws Exception {
        compareAndOrOperators("&&");
        compareAndOrOperators("||");
    }

    @Test
    public void testBinaryOperators() throws Exception {
        compareBinaryOperators("&");
        compareBinaryOperators("|");
        compareBinaryOperators("^");
    }

    @Test
    public void testTernaryOperator() throws Exception {
        TernaryOpContext expression = parseExpression("true ? 1 : 0");
        assertEquals(5, expression.getChildCount());
        assertEquals("true",
                ((PrimaryContext) expression.left).literal().javaLiteral().getText());
        assertEquals("?", expression.op.getText());
        assertEquals("1",
                ((PrimaryContext) expression.iftrue).literal().javaLiteral().getText());
        assertEquals(":", expression.getChild(3).getText());
        assertEquals("0", ((PrimaryContext) expression.iffalse).literal().javaLiteral().getText());
    }

    @Test
    public void testDot() throws Exception {
        DotOpContext expression = parseExpression("one.two.three");
        assertEquals(3, expression.getChildCount());
        assertEquals("three", expression.Identifier().getText());
        assertEquals(".", expression.getChild(1).getText());
        DotOpContext left = (DotOpContext) expression.expression();
        assertEquals("two", left.Identifier().getText());
        assertEquals(".", left.getChild(1).getText());
        assertEquals("one", ((PrimaryContext) left.expression()).identifier().getText());
    }

    @Test
    public void testQuestionQuestion() throws Exception {
        QuestionQuestionOpContext expression = parseExpression("one ?? two");
        assertEquals(3, expression.getChildCount());
        assertEquals("one", ((PrimaryContext) expression.left).identifier().getText());
        assertEquals("two", ((PrimaryContext) expression.right).identifier().getText());
        assertEquals("??", expression.op.getText());
    }

    @Test
    public void testResourceReference() throws Exception {
        compareResource("@id/foo_bar");
        compareResource("@transition/foo_bar");
        compareResource("@anim/foo_bar");
        compareResource("@animator/foo_bar");
        compareResource("@android:id/foo_bar");
        compareResource("@app:id/foo_bar");
    }

    @Test
    public void testDefaults() throws Exception {
        BindingSyntaxContext syntax = parseExpressionString("foo.bar, default = @id/foo_bar");
        BindingExpressionParser.DefaultsContext defaults = syntax
                .accept(new BindingExpressionBaseVisitor<DefaultsContext>() {
                    @Override
                    public BindingExpressionParser.DefaultsContext visitDefaults(
                            @NotNull BindingExpressionParser.DefaultsContext ctx) {
                        return ctx;
                    }
                });
        assertEquals("@id/foo_bar", defaults.constantValue().ResourceReference().getText());
    }

    @Test
    public void testParentheses() throws Exception {
        GroupingContext grouping = parseExpression("(1234)");
        assertEquals("1234", grouping.expression().getText());
    }

    // ---------------------- Helpers --------------------

    private void compareResource(String value) throws Exception {
        ResourceContext resourceContext = parseExpression(value);
        assertEquals(value, resourceContext.getText());
    }

    private void compareUnaryOperators(String op) throws Exception {
        UnaryOpContext expression = parseExpression(op + " 2");
        assertEquals(2, expression.getChildCount());
        assertEquals(op, expression.op.getText());
        assertEquals("2",
                ((PrimaryContext) expression.expression()).literal().javaLiteral()
                        .getText());
    }

    private void compareBinaryOperators(String op) throws Exception {
        BinaryOpContext expression = parseExpression("1 " + op + " 2");
        assertEquals(3, expression.getChildCount());
        assertTrue(expression.left instanceof ExpressionContext);
        String one = ((PrimaryContext) expression.left).literal().javaLiteral().getText();
        assertEquals("1", one);
        assertEquals(op, expression.op.getText());
        assertTrue(expression.right instanceof ExpressionContext);
        String two = ((PrimaryContext) expression.right).literal().javaLiteral().getText();
        assertEquals("2", two);
    }

    private void compareMathOperators(String op) throws Exception {
        MathOpContext expression = parseExpression("1 " + op + " 2");
        assertEquals(3, expression.getChildCount());
        assertTrue(expression.left instanceof ExpressionContext);
        String one = ((PrimaryContext) expression.left).literal().javaLiteral().getText();
        assertEquals("1", one);
        assertEquals(op, expression.op.getText());
        assertTrue(expression.right instanceof ExpressionContext);
        String two = ((PrimaryContext) expression.right).literal().javaLiteral().getText();
        assertEquals("2", two);
    }

    private void compareBitShiftOperators(String op) throws Exception {
        BitShiftOpContext expression = parseExpression("1 " + op + " 2");
        assertEquals(3, expression.getChildCount());
        assertTrue(expression.left instanceof ExpressionContext);
        String one = ((PrimaryContext) expression.left).literal().javaLiteral().getText();
        assertEquals("1", one);
        assertEquals(op, expression.op.getText());
        assertTrue(expression.right instanceof ExpressionContext);
        String two = ((PrimaryContext) expression.right).literal().javaLiteral().getText();
        assertEquals("2", two);
    }

    private void compareComparisonOperators(String op) throws Exception {
        ComparisonOpContext expression = parseExpression("1 " + op + " 2");
        assertEquals(3, expression.getChildCount());
        assertTrue(expression.left instanceof ExpressionContext);
        String one = ((PrimaryContext) expression.left).literal().javaLiteral().getText();
        assertEquals("1", one);
        assertEquals(op, expression.op.getText());
        assertTrue(expression.right instanceof ExpressionContext);
        String two = ((PrimaryContext) expression.right).literal().javaLiteral().getText();
        assertEquals("2", two);
    }

    private void compareAndOrOperators(String op) throws Exception {
        AndOrOpContext expression = parseExpression("1 " + op + " 2");
        assertEquals(3, expression.getChildCount());
        assertTrue(expression.left instanceof ExpressionContext);
        String one = ((PrimaryContext) expression.left).literal().javaLiteral().getText();
        assertEquals("1", one);
        assertEquals(op, expression.op.getText());
        assertTrue(expression.right instanceof ExpressionContext);
        String two = ((PrimaryContext) expression.right).literal().javaLiteral().getText();
        assertEquals("2", two);
    }

    private void compareIntLiteral(String constant) throws Exception {
        LiteralContext literal = parseLiteral(constant);
        String token = literal.javaLiteral().getText();
        assertEquals(constant, token);
    }

    private void compareFloatLiteral(String constant) throws Exception {
        LiteralContext literal = parseLiteral(constant);
        String token = literal.javaLiteral().getText();
        assertEquals(constant, token);
    }

    private void compareBoolLiteral(String constant) throws Exception {
        LiteralContext literal = parseLiteral(constant);
        String token = literal.javaLiteral().getText();
        assertEquals(constant, token);
    }

    private BindingSyntaxContext parse(String value) throws Exception {
        return parseExpressionString(value);
    }

    private <T extends ExpressionContext> T parseExpression(String value) throws Exception {
        return (T) parse(value).accept(new BindingExpressionBaseVisitor<ExpressionContext>() {
            @Override
            public ExpressionContext visitRootExpr(
                    @NotNull BindingExpressionParser.RootExprContext ctx) {
                return ctx.expression();
            }
        });
    }

    private PrimaryContext parsePrimary(String value) throws Exception {
        return parseExpression(value);
    }

    private LiteralContext parseLiteral(String value) throws Exception {
        return parsePrimary(value).literal();
    }

    BindingExpressionParser.BindingSyntaxContext parseExpressionString(String s) throws Exception {
        ANTLRInputStream input = new ANTLRInputStream(new StringReader(s));
        BindingExpressionLexer lexer = new BindingExpressionLexer(input);
        CommonTokenStream tokens = new CommonTokenStream(lexer);
        BindingExpressionParser parser = new BindingExpressionParser(tokens);
        return parser.bindingSyntax();
    }
}