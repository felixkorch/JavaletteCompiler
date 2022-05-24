/* -*- c++ -*- File generated by the BNF Converter (bnfc 2.9.4). */

/* Parser definition to be used with Bison. */

/* Generate header file for lexer. */
%defines "bnfc/Bison.H"
%name-prefix = "bnfc"
  /* From Bison 2.6: %define api.prefix {bnfc} */

/* Reentrant parser */
%pure_parser
  /* From Bison 2.3b (2008): %define api.pure full */
%lex-param   { yyscan_t scanner }
%parse-param { yyscan_t scanner }

/* Turn on line/column tracking in the bnfclloc structure: */
%locations

/* Argument to the parser to be filled with the parsed tree. */
%parse-param { YYSTYPE *result }

%{
/* Begin C preamble code */

#include <algorithm> /* for std::reverse */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Absyn.H"

#define YYMAXDEPTH 10000000

/* The type yyscan_t is defined by flex, but we need it in the parser already. */
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE bnfc_scan_string(const char *str, yyscan_t scanner);
extern void bnfc_delete_buffer(YY_BUFFER_STATE buf, yyscan_t scanner);

extern void bnfclex_destroy(yyscan_t scanner);
extern char* bnfcget_text(yyscan_t scanner);

extern yyscan_t bnfc_initialize_lexer(FILE * inp);

/* End C preamble code */
%}

%union
{
  int    _int;
  char   _char;
  double _double;
  char*  _string;
  bnfc::Prog* prog_;
  bnfc::TopDef* topdef_;
  bnfc::ListTopDef* listtopdef_;
  bnfc::Arg* arg_;
  bnfc::ListArg* listarg_;
  bnfc::Blk* blk_;
  bnfc::ListStmt* liststmt_;
  bnfc::Stmt* stmt_;
  bnfc::Item* item_;
  bnfc::ListItem* listitem_;
  bnfc::Dim* dim_;
  bnfc::Type* type_;
  bnfc::ListType* listtype_;
  bnfc::ListDim* listdim_;
  bnfc::ExpDim* expdim_;
  bnfc::Expr* expr_;
  bnfc::ListExpr* listexpr_;
  bnfc::ListExpDim* listexpdim_;
  bnfc::AddOp* addop_;
  bnfc::MulOp* mulop_;
  bnfc::RelOp* relop_;
}

%{
void yyerror(YYLTYPE *loc, yyscan_t scanner, YYSTYPE *result, const char *msg)
{
  fprintf(stderr, "ERROR: %d,%d: %s at %s\n",
    loc->first_line, loc->first_column, msg, bnfcget_text(scanner));
}

int yyparse(yyscan_t scanner, YYSTYPE *result);

extern int yylex(YYSTYPE *lvalp, YYLTYPE *llocp, yyscan_t scanner);
%}

%token          _ERROR_
%token          _BANG        /* ! */
%token          _BANGEQ      /* != */
%token          _PERCENT     /* % */
%token          _DAMP        /* && */
%token          _LPAREN      /* ( */
%token          _RPAREN      /* ) */
%token          _STAR        /* * */
%token          _PLUS        /* + */
%token          _DPLUS       /* ++ */
%token          _COMMA       /* , */
%token          _MINUS       /* - */
%token          _DMINUS      /* -- */
%token          _DOT         /* . */
%token          _SLASH       /* / */
%token          _COLON       /* : */
%token          _SEMI        /* ; */
%token          _LT          /* < */
%token          _LDARROW     /* <= */
%token          _EQ          /* = */
%token          _DEQ         /* == */
%token          _GT          /* > */
%token          _GTEQ        /* >= */
%token          _LBRACK      /* [ */
%token          _EMPTYBRACK  /* [] */
%token          _RBRACK      /* ] */
%token          _KW_boolean  /* boolean */
%token          _KW_double   /* double */
%token          _KW_else     /* else */
%token          _KW_false    /* false */
%token          _KW_for      /* for */
%token          _KW_if       /* if */
%token          _KW_int      /* int */
%token          _KW_new      /* new */
%token          _KW_return   /* return */
%token          _KW_true     /* true */
%token          _KW_void     /* void */
%token          _KW_while    /* while */
%token          _LBRACE      /* { */
%token          _DBAR        /* || */
%token          _RBRACE      /* } */
%token<_string> _STRING_
%token<_int>    _INTEGER_
%token<_double> _DOUBLE_
%token<_string> _IDENT_

%type <prog_> Prog
%type <topdef_> TopDef
%type <listtopdef_> ListTopDef
%type <arg_> Arg
%type <listarg_> ListArg
%type <blk_> Blk
%type <liststmt_> ListStmt
%type <stmt_> Stmt
%type <item_> Item
%type <listitem_> ListItem
%type <dim_> Dim
%type <type_> Type1
%type <type_> Type
%type <listtype_> ListType
%type <listdim_> ListDim
%type <expdim_> ExpDim
%type <expr_> Expr8
%type <expr_> Expr7
%type <expr_> Expr6
%type <expr_> Expr5
%type <expr_> Expr4
%type <expr_> Expr3
%type <expr_> Expr2
%type <expr_> Expr1
%type <expr_> Expr
%type <listexpr_> ListExpr
%type <listexpdim_> ListExpDim
%type <addop_> AddOp
%type <mulop_> MulOp
%type <relop_> RelOp

%start Prog

%%

Prog : ListTopDef { std::reverse($1->begin(),$1->end()) ;$$ = new bnfc::Program($1); $$->line_number = @$.first_line; $$->char_number = @$.first_column; result->prog_ = $$; }
;
TopDef : Type _IDENT_ _LPAREN ListArg _RPAREN Blk { std::reverse($4->begin(),$4->end()) ;$$ = new bnfc::FnDef($1, $2, $4, $6); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
ListTopDef : TopDef { $$ = new bnfc::ListTopDef(); $$->push_back($1); }
  | TopDef ListTopDef { $2->push_back($1); $$ = $2; }
;
Arg : Type _IDENT_ { $$ = new bnfc::Argument($1, $2); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
ListArg : /* empty */ { $$ = new bnfc::ListArg(); }
  | Arg { $$ = new bnfc::ListArg(); $$->push_back($1); }
  | Arg _COMMA ListArg { $3->push_back($1); $$ = $3; }
;
Blk : _LBRACE ListStmt _RBRACE { $$ = new bnfc::Block($2); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
ListStmt : /* empty */ { $$ = new bnfc::ListStmt(); }
  | ListStmt Stmt { $1->push_back($2); $$ = $1; }
;
Stmt : _SEMI { $$ = new bnfc::Empty(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | Blk { $$ = new bnfc::BStmt($1); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | Type ListItem _SEMI { std::reverse($2->begin(),$2->end()) ;$$ = new bnfc::Decl($1, $2); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | Expr _EQ Expr _SEMI { $$ = new bnfc::Ass($1, $3); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _IDENT_ _DPLUS _SEMI { $$ = new bnfc::Incr($1); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _IDENT_ _DMINUS _SEMI { $$ = new bnfc::Decr($1); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _KW_return Expr _SEMI { $$ = new bnfc::Ret($2); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _KW_return _SEMI { $$ = new bnfc::VRet(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _KW_if _LPAREN Expr _RPAREN Stmt { $$ = new bnfc::Cond($3, $5); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _KW_if _LPAREN Expr _RPAREN Stmt _KW_else Stmt { $$ = new bnfc::CondElse($3, $5, $7); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _KW_while _LPAREN Expr _RPAREN Stmt { $$ = new bnfc::While($3, $5); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _KW_for _LPAREN Type _IDENT_ _COLON Expr _RPAREN Stmt { $$ = new bnfc::For($3, $4, $6, $8); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | Expr _SEMI { $$ = new bnfc::SExp($1); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
Item : _IDENT_ { $$ = new bnfc::NoInit($1); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _IDENT_ _EQ Expr { $$ = new bnfc::Init($1, $3); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
ListItem : Item { $$ = new bnfc::ListItem(); $$->push_back($1); }
  | Item _COMMA ListItem { $3->push_back($1); $$ = $3; }
;
Dim : _EMPTYBRACK { $$ = new bnfc::Dimension(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
Type1 : _KW_int { $$ = new bnfc::Int(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _KW_double { $$ = new bnfc::Doub(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _KW_boolean { $$ = new bnfc::Bool(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _KW_void { $$ = new bnfc::Void(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _LPAREN Type _RPAREN { $$ = $2; $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
Type : Type1 ListDim { std::reverse($2->begin(),$2->end()) ;$$ = new bnfc::Arr($1, $2); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | Type1 { $$ = $1; $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
ListType : /* empty */ { $$ = new bnfc::ListType(); }
  | Type { $$ = new bnfc::ListType(); $$->push_back($1); }
  | Type _COMMA ListType { $3->push_back($1); $$ = $3; }
;
ListDim : Dim { $$ = new bnfc::ListDim(); $$->push_back($1); }
  | Dim ListDim { $2->push_back($1); $$ = $2; }
;
ExpDim : _LBRACK Expr _RBRACK { $$ = new bnfc::ExpDimen($2); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
Expr8 : Expr8 ExpDim { $$ = new bnfc::EIndex($1, $2); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _IDENT_ { $$ = new bnfc::EVar($1); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _IDENT_ _LPAREN ListExpr _RPAREN { std::reverse($3->begin(),$3->end()) ;$$ = new bnfc::EApp($1, $3); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _LPAREN Expr _RPAREN { $$ = $2; $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
Expr7 : _KW_new Type ListExpDim { $$ = new bnfc::EArrNew($2, $3); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | Expr8 { $$ = $1; $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
Expr6 : Expr7 _DOT _IDENT_ { $$ = new bnfc::EArrLen($1, $3); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _INTEGER_ { $$ = new bnfc::ELitInt($1); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _DOUBLE_ { $$ = new bnfc::ELitDoub($1); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _KW_true { $$ = new bnfc::ELitTrue(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _KW_false { $$ = new bnfc::ELitFalse(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _STRING_ { $$ = new bnfc::EString($1); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | Expr7 { $$ = $1; $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
Expr5 : _MINUS Expr6 { $$ = new bnfc::Neg($2); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _BANG Expr6 { $$ = new bnfc::Not($2); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | Expr6 { $$ = $1; $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
Expr4 : Expr4 MulOp Expr5 { $$ = new bnfc::EMul($1, $2, $3); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | Expr5 { $$ = $1; $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
Expr3 : Expr3 AddOp Expr4 { $$ = new bnfc::EAdd($1, $2, $3); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | Expr4 { $$ = $1; $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
Expr2 : Expr2 RelOp Expr3 { $$ = new bnfc::ERel($1, $2, $3); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | Expr3 { $$ = $1; $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
Expr1 : Expr2 _DAMP Expr1 { $$ = new bnfc::EAnd($1, $3); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | Expr2 { $$ = $1; $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
Expr : Expr1 _DBAR Expr { $$ = new bnfc::EOr($1, $3); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | Expr1 { $$ = $1; $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
ListExpr : /* empty */ { $$ = new bnfc::ListExpr(); }
  | Expr { $$ = new bnfc::ListExpr(); $$->push_back($1); }
  | Expr _COMMA ListExpr { $3->push_back($1); $$ = $3; }
;
ListExpDim : /* empty */ { $$ = new bnfc::ListExpDim(); }
  | ListExpDim ExpDim { $1->push_back($2); $$ = $1; }
;
AddOp : _PLUS { $$ = new bnfc::Plus(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _MINUS { $$ = new bnfc::Minus(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
MulOp : _STAR { $$ = new bnfc::Times(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _SLASH { $$ = new bnfc::Div(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _PERCENT { $$ = new bnfc::Mod(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;
RelOp : _LT { $$ = new bnfc::LTH(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _LDARROW { $$ = new bnfc::LE(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _GT { $$ = new bnfc::GTH(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _GTEQ { $$ = new bnfc::GE(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _DEQ { $$ = new bnfc::EQU(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
  | _BANGEQ { $$ = new bnfc::NE(); $$->line_number = @$.first_line; $$->char_number = @$.first_column; }
;

%%

namespace bnfc
{
/* Entrypoint: parse Prog* from file. */
Prog* pProg(FILE *inp)
{
  YYSTYPE result;
  yyscan_t scanner = bnfc_initialize_lexer(inp);
  if (!scanner) {
    fprintf(stderr, "Failed to initialize lexer.\n");
    return 0;
  }
  int error = yyparse(scanner, &result);
  bnfclex_destroy(scanner);
  if (error)
  { /* Failure */
    return 0;
  }
  else
  { /* Success */
    return result.prog_;
  }
}

/* Entrypoint: parse Prog* from string. */
Prog* psProg(const char *str)
{
  YYSTYPE result;
  yyscan_t scanner = bnfc_initialize_lexer(0);
  if (!scanner) {
    fprintf(stderr, "Failed to initialize lexer.\n");
    return 0;
  }
  YY_BUFFER_STATE buf = bnfc_scan_string(str, scanner);
  int error = yyparse(scanner, &result);
  bnfc_delete_buffer(buf, scanner);
  bnfclex_destroy(scanner);
  if (error)
  { /* Failure */
    return 0;
  }
  else
  { /* Success */
    return result.prog_;
  }
}


}
