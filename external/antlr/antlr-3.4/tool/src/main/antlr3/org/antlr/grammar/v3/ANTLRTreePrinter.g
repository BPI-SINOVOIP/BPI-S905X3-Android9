/*
 [The "BSD license"]
 Copyright (c) 2005-2011 Terence Parr
 All rights reserved.

 Grammar conversion to ANTLR v3:
 Copyright (c) 2011 Sam Harwell
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 1. Redistributions of source code must retain the above copyright
	notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.
 3. The name of the author may not be used to endorse or promote products
	derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/** Print out a grammar (no pretty printing).
 *
 *  Terence Parr
 *  University of San Francisco
 *  August 19, 2003
 */
tree grammar ANTLRTreePrinter;

options
{
	tokenVocab = ANTLR;
	ASTLabelType = GrammarAST;
}

@header {
package org.antlr.grammar.v3;
import org.antlr.tool.*;
import java.util.StringTokenizer;
}

@members {
protected Grammar grammar;
protected boolean showActions;
protected StringBuilder buf = new StringBuilder(300);

private ANTLRTreePrinter.block_return block(GrammarAST t, boolean forceParens) throws RecognitionException {
    ANTLRTreePrinter other = new ANTLRTreePrinter(new CommonTreeNodeStream(t));
    other.buf = buf;
    return other.block(forceParens);
}

public final int countAltsForBlock(GrammarAST t) {
    int n = 0;
    for ( int i = 0; i < t.getChildCount(); i++ )
    {
        if ( t.getChild(i).getType() == ALT )
            n++;
    }

    return n;
}

public void out(String s) {
    buf.append(s);
}

@Override
public void reportError(RecognitionException ex) {
    Token token = null;
    if (ex instanceof MismatchedTokenException) {
        token = ((MismatchedTokenException)ex).token;
    } else if (ex instanceof NoViableAltException) {
        token = ((NoViableAltException)ex).token;
    }

    ErrorManager.syntaxError(
        ErrorManager.MSG_SYNTAX_ERROR,
        grammar,
        token,
        "antlr.print: " + ex.toString(),
        ex );
}

/** Normalize a grammar print out by removing all double spaces
 *  and trailing/beginning stuff.  FOr example, convert
 *
 *  ( A  |  B  |  C )*
 *
 *  to
 *
 *  ( A | B | C )*
 */
public static String normalize(String g) {
    StringTokenizer st = new StringTokenizer(g, " ", false);
    StringBuffer buf = new StringBuffer();
    while ( st.hasMoreTokens() ) {
        String w = st.nextToken();
        buf.append(w);
        buf.append(" ");
    }
    return buf.toString().trim();
}
}

/** Call this to figure out how to print */
public
toString[Grammar g, boolean showActions] returns [String s=null]
@init {
	grammar = g;
	this.showActions = showActions;
}
	:	(	grammar_
		|	rule
		|	alternative
		|	element
		|	single_rewrite
		|	rewrite
		|	EOR //{s="EOR";}
		)
		{return normalize(buf.toString());}
	;

// --------------

grammar_
	:	^( LEXER_GRAMMAR grammarSpec["lexer " ] )
	|	^( PARSER_GRAMMAR grammarSpec["parser "] )
	|	^( TREE_GRAMMAR grammarSpec["tree "] )
	|	^( COMBINED_GRAMMAR grammarSpec[""] )
	;

attrScope
	:	^( 'scope' ID ruleAction* ACTION )
	;

grammarSpec[String gtype]
	:	id=ID {out(gtype+"grammar "+$id.text);}
		(cmt=DOC_COMMENT {out($cmt.text+"\n");} )?
		(optionsSpec)? {out(";\n");}
		(delegateGrammars)?
		(tokensSpec)?
		(attrScope)*
		(actions)?
		rules
	;

actions
	:	( action )+
	;

action
@init {
	String scope=null, name=null;
	String action=null;
}
	:	^(	AMPERSAND id1=ID
			(	id2=ID a1=ACTION
				{scope=$id1.text; name=$a1.text; action=$a1.text;}
			|	a2=ACTION
				{scope=null; name=$id1.text; action=$a2.text;}
			)
		)
		{
			if ( showActions )
			{
				out("@"+(scope!=null?scope+"::":"")+name+action);
			}
		}
	;

optionsSpec
	:	^(	OPTIONS {out(" options {");}
			(option {out("; ");})+
			{out("} ");}
		)
	;

option
	:	^( ASSIGN id=ID {out($id.text+"=");} optionValue )
	;

optionValue
	:	id=ID            {out($id.text);}
	|	s=STRING_LITERAL {out($s.text);}
	|	c=CHAR_LITERAL   {out($c.text);}
	|	i=INT            {out($i.text);}
//	|   charSet
	;

/*
charSet
	:   #( CHARSET charSetElement )
	;

charSetElement
	:   c:CHAR_LITERAL {out(#c.getText());}
	|   #( OR c1:CHAR_LITERAL c2:CHAR_LITERAL )
	|   #( RANGE c3:CHAR_LITERAL c4:CHAR_LITERAL )
	;
*/

delegateGrammars
	:	^( 'import' ( ^(ASSIGN ID ID) | ID )+ )
	;

tokensSpec
	:	^(TOKENS tokenSpec*)
	;

tokenSpec
	:	TOKEN_REF
	|	^( ASSIGN TOKEN_REF (STRING_LITERAL|CHAR_LITERAL) )
	;

rules
	:	( rule | precRule )+
	;

rule
	:	^(	RULE id=ID
			(modifier)?
			{out($id.text);}
			^(ARG (arg=ARG_ACTION {out("["+$arg.text+"]");} )? )
			^(RET (ret=ARG_ACTION {out(" returns ["+$ret.text+"]");} )? )
			(throwsSpec)?
			(optionsSpec)?
			(ruleScopeSpec)?
			(ruleAction)*
			{out(" :");}
			{
				if ( input.LA(5) == NOT || input.LA(5) == ASSIGN )
					out(" ");
			}
			b=block[false]
			(exceptionGroup)?
			EOR {out(";\n");}
		)
	;

precRule
	:	^(	PREC_RULE id=ID
			(modifier)?
			{out($id.text);}
			^(ARG (arg=ARG_ACTION {out("["+$arg.text+"]");} )? )
			^(RET (ret=ARG_ACTION {out(" returns ["+$ret.text+"]");} )? )
			(throwsSpec)?
			(optionsSpec)?
			(ruleScopeSpec)?
			(ruleAction)*
			{out(" :");}
			{
				if ( input.LA(5) == NOT || input.LA(5) == ASSIGN )
					out(" ");
			}
			b=block[false]
			(exceptionGroup)?
			EOR {out(";\n");}
		)
	;

ruleAction
	:	^(AMPERSAND id=ID a=ACTION )
		{if ( showActions ) out("@"+$id.text+"{"+$a.text+"}");}
	;

modifier
@init
{out($modifier.start.getText()); out(" ");}
	:	'protected'
	|	'public'
	|	'private'
	|	'fragment'
	;

throwsSpec
	:	^('throws' ID+)
	;

ruleScopeSpec
	:	^( 'scope' ruleAction* (ACTION)? ( ID )* )
	;

block[boolean forceParens]
@init
{
int numAlts = countAltsForBlock($start);
}
	:	^(	BLOCK
			{
				if ( forceParens||numAlts>1 )
				{
					//for ( Antlr.Runtime.Tree.Tree parent = $start.getParent(); parent != null && parent.getType() != RULE; parent = parent.getParent() )
					//{
					//	if ( parent.getType() == BLOCK && countAltsForBlock((GrammarAST)parent) > 1 )
					//	{
					//		out(" ");
					//		break;
					//	}
					//}
					out(" (");
				}
			}
			(optionsSpec {out(" :");} )?
			alternative rewrite ( {out("|");} alternative rewrite )*
			EOB   {if ( forceParens||numAlts>1 ) out(")");}
		 )
	;

alternative
	:	^( ALT element* EOA )
	;

exceptionGroup
	:	( exceptionHandler )+ (finallyClause)?
	|	finallyClause
	;

exceptionHandler
	:	^('catch' ARG_ACTION ACTION)
	;

finallyClause
	:	^('finally' ACTION)
	;

rewrite
	:	^(REWRITES single_rewrite+)
	|	REWRITES
	|
	;

single_rewrite
	:	^(	REWRITE {out(" ->");}
			(	SEMPRED {out(" {"+$SEMPRED.text+"}?");}
			)?
			(	alternative
			|	rewrite_template
			|	ETC {out("...");}
			|	ACTION {out(" {"+$ACTION.text+"}");}
			)
		)
	;

rewrite_template
	:	^(	TEMPLATE
			(	id=ID {out(" "+$id.text);}
			|	ind=ACTION {out(" ({"+$ind.text+"})");}
			)
			^(	ARGLIST
				{out("(");}
				(	^(	ARG arg=ID {out($arg.text+"=");}
						a=ACTION   {out($a.text);}
					)
				)*
				{out(")");}
			)
			(	DOUBLE_QUOTE_STRING_LITERAL {out(" "+$DOUBLE_QUOTE_STRING_LITERAL.text);}
			|	DOUBLE_ANGLE_STRING_LITERAL {out(" "+$DOUBLE_ANGLE_STRING_LITERAL.text);}
			)?
		)
	;

element
	:	^(ROOT element) {out("^");}
	|	^(BANG element) {out("!");}
	|	atom
	|	^(NOT {out("~");} element)
	|	^(RANGE atom {out("..");} atom)
	|	^(CHAR_RANGE atom {out("..");} atom)
	|	^(ASSIGN id=ID {out($id.text+"=");} element)
	|	^(PLUS_ASSIGN id2=ID {out($id2.text+"+=");} element)
	|	ebnf
	|	tree_
	|	^( SYNPRED block[true] ) {out("=>");}
	|	a=ACTION  {if ( showActions ) {out("{"); out($a.text); out("}");}}
	|	a2=FORCED_ACTION  {if ( showActions ) {out("{{"); out($a2.text); out("}}");}}
	|	pred=SEMPRED
		{
			if ( showActions )
			{
				out("{");
				out($pred.text);
				out("}?");
			}
			else
			{
				out("{...}?");
			}
		}
	|	spred=SYN_SEMPRED
		{
			String name = $spred.text;
			GrammarAST predAST=grammar.getSyntacticPredicate(name);
			block(predAST, true);
			out("=>");
		}
	|	^(BACKTRACK_SEMPRED .*) // don't print anything (auto backtrack stuff)
	|	gpred=GATED_SEMPRED
		{
		if ( showActions ) {out("{"); out($gpred.text); out("}? =>");}
		else {out("{...}? =>");}
		}
	|	EPSILON
	;

ebnf
	:	block[true] {out(" ");}
	|	^( OPTIONAL block[true] ) {out("? ");}
	|	^( CLOSURE block[true] )  {out("* ");}
	|	^( POSITIVE_CLOSURE block[true] ) {out("+ ");}
	;

tree_
	:	^(TREE_BEGIN {out(" ^(");} element (element)* {out(") ");} )
	;

atom
@init
{out(" ");}
	:	(	^(	RULE_REF		{out($start.toString());}
				(rarg=ARG_ACTION	{out("["+$rarg.toString()+"]");})?
				(ast_suffix)?
			)
		|	^(	TOKEN_REF		{out($start.toString());}
				(targ=ARG_ACTION	{out("["+$targ.toString()+"]");} )?
				(ast_suffix)?
			)
		|	^(	CHAR_LITERAL	{out($start.toString());}
				(ast_suffix)?
			)
		|	^(	STRING_LITERAL	{out($start.toString());}
				(ast_suffix)?
			)
		|	^(	WILDCARD		{out($start.toString());}
				(ast_suffix)?
			)
		)
		{out(" ");}
	|	LABEL {out(" $"+$LABEL.text);} // used in -> rewrites
	|	^(DOT ID {out($ID.text+".");} atom) // scope override on rule
	;

ast_suffix
	:	ROOT {out("^");}
	|	BANG  {out("!");}
	;
