#pragma once

#include "language/tokenizer.hpp"
#include "language/parser.hpp"
#include "language/interpretor.hpp"

namespace wolfscript
{

/*

Typing

  This language is mainly statically typed meaning that a variable declared with an int will always be an int.
However, soft typing through implicit variable declarations will allow basic generics.

Variables

  Variable declaration is inspired by Go in that they start with 'var', proceeded
by an identifier and an optional type.

// Defines a variable of type int
var myvariable int;

// A variable of type float initialized with the value 34
var myvariable2 float = 34;

// Defines a variable that is implicitly of type int.
// Its very simple deduction based on the result of the expression.
// This could also mean that the type of the expression can change
// at different points during runtime hence the soft typing mentioned before.
var myimplicitvariables = 34;


Functions

  Functions are fairly standard and lambda/anonymous functions are very similar in syntax.
Due to the nature of the language, you can define a function at any scope.

// A declaration of a function with an int and string parameter
// and a return type of int.
function myfunction(arg1 int, arg2 string) int
{
	return 5; // Return is very C-like
}

// Same function but as an anonymous function.
// Note: this example takes advantage of implicit typing.
var myfunction = function(arg1 int, arg2 string) int
{
	return 5;
};

// Same as example above but with explicit typing
var myfunction function(int, string) int = function(arg1 int, arg2 string) int
{
	return 5;
};

Classes

  Classes are shared objects meaning they are always passed around as a reference and
are destroyed when they are no longer referenced. If you want to create a value type,
create a struct instead.

class myclass
{
	// All methods and members are public by default
	
	// Contructor
	this()
	{
	}

	// Deconstructor
	~this()
	{
	}

	function mypublicmethod()
	{
	}

	private function myprivatemethod()
	{
	}

	var mypublicmember int;
	private var myprivatemember int;
}

// Initialization of a class object is just
// like calling a function.
// Note: This is using implicit typing
//   for convenience.
var myvar = myclass();

// myvar and myvar2 now reference the same object
var myvar2 myclass = myvar;

// Initializes this reference to null
// instead of creating a new myclass object.
var myvar3 myclass;

Structs

  Structs have all the features (members, methods, etc...) of classes except they are
treated like values and passed by value.

struct mystruct
{
}

// Creates a struct object
var myvar mystruct;

// myvar is copied into myvar2
var myvar2 = myvar;


Referencing value types

  You can refernce values by placing 'ref' after the type name
in a variable declaration. This can also make the lifetime of a
value go beyond its scope.

var myvar int = 23;

// myvar and myvarref now point to the same object
var myvarref int ref = myvar;

// Implicit typing
var myvarref ref = myref;

// refparam references an int with read-write access
// while crefparam has read-only access
function myfunction(refparam int ref, const crefparam int ref)
{
	
}

A rough sketch of the language's grammer (Note: This may be out of data):

<program> : <statement-list>

<statement-list> :  ( <statement> )*

<compound-statement> : '{' ( <statement> )* '}'

<statement> : <compound-statement> 
		    | <var-definition>
		    | <if-statement>
		    | <function-definition>
		    | <expression> <eol>
			| <eol>

<namespace-statement> : 'namespace' <namespace-identifier> <compound-statement>

<function-definition> : 'function' <identifier> '(' [ <param> { ',' <param> } ] ')' <compound-statement>

<if-statement> : 'if' '(' <expression> ')' <statement> [ 'else if' '(' <expression> ')' <statement> ]* [ 'else' <statement> ]

<try-statement> : 'try' block { catchstatment }
<catch-statment> : 'catch' '(' [ param { ',' param } ] ')' '{' block '}'

<param> : <identifier> [<typename>]

<class-definition> : 'class' <identifier> [ ':' <identifier> [ ',' <identifier> ]* ] '{' <class-body>* '}'

<class-body> : [ <class-access-modifiers> ] [ 'const' ] <function-definition>
             | [ <class-access-modifiers> ] <var-definition>

<class-access-modifiers> : 'public'
                         | 'private'
						 | 'protected'

<var-definition> : ('var' | 'const') <identifier> '=' <expr> <eol>
                 | ('var' | 'const') <identifier> <typename> [ '=' <expr> ] <eol>

<expression> : <assignment-expression>

<assignment-expression> : <ternay-expression> ( <assignment-operators> <ternay-expression> )*

<ternay-expression> : <logical-or-expression> ( '?' <logical-or-expression> ':' <ternay-expression> )

<logical-or-expression> : <logical-and-expression> ( '||' <logical-and-expression> )*

<logical-and-expression> : <assignment-expression> ( '&&' <assignment-expression> )*

<inclusive-or-expression> : <exclusive-or-expression> ( '|' <exclusive-or-expression> )*

<exclusive-or-expression> : <and-expression> ( '^' <and-expression> )*

<and-expression> : <equality-expression> ( '&' <equality-expression> )*

<equality-expression> : <relational-expression> ( ('==' | '!=') <relational-expression> )*

<relational-expression> :: <shift-expression> ( ('<' | '>' | '<=' | '>=') <shift-expression> )*

<shift-expression> ::= <additive-expression> ( ('<<' | '>>') <additive-expression>  )*

<additive-expression> : <multiplicative-expression> ( ('+' | '-') <multiplicative-expression> )*

<multiplicative-expression> : <member-accessor> ( ('*' | '/') <member-accessor> )*

<member-accessor> : <cast-expression> ( '.' <cast-expression> )*

<cast-expression> : <unary-expression>
                  | '(' <namespace-identifier> ')' <cast-expression>

<unary-expression> : <postfix-expression>
                   | ('++' | --') <unary-expression>

<postfix-expression> : <factor> ( '.' <identifier> )*

<factor> : '(' <expression> ')'
	     | <constant>
	     | <namespace-identifier>
	     | <function-call>
		 | <anonymous-function>

(* This will actually generate a class object with the '()' operator overloaded and private members as captures. *)
<anonymous-function> : 'function' (' [ <param> ( ',' <param> )* ] ')' <compound-statement>

<function-call> : <namespace-identifier> '(' [ <expr> [ ',' <expr> ]* ] ')'
                | <anonymous-function> '(' [ <expr> [ ',' <expr> ]* ] ')'

<namespace-identifier> : <identifier> ( '::' <identifier> )*

<identifier> : (* C-like *)

<assignment-operators> : '='
                       | '+='
					   | '-='
					   | '*='
					   | '/='
					   | '%='
					   | '>>='
					   | '<<='
					   | '&='
					   | '|='
					   | '^='

<prefix-unary-operators> : '+'
                         | '-'

<typename> : int
           | uint
		   | float
		   | double
		   | string
		   | <namespace-identifier>

<constant> : integer
           | float
		   | string

<eol> : ';'
*/


}
