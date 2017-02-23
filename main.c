#include <stdio.h>
#include "mpc.h"
#include "d-function.h"

char * readline(char * read){
	fputs(read,stdout);
	char *in=(char *)malloc(sizeof(char)*50);
	gets(in);
	return in ;
	
}


long eval(mpc_ast_t *t){

if(strstr(t->tag,"number"))
	return atoi(t->contents);

char *op=t->children[1]->contents;
//printf("%c \n",*op);
long ctal=eval(t->children[2]);

int i=3;
while(strstr(t->children[i]->tag,"expr"))
{
	ctal=eval_op(ctal,op,eval(t->children[i]));
	i++;
}
return ctal;

}

long eval_op(long x,char  *op,long y){
 switch (*op)
	{
	case'+':
		return x + y;
	case'-':
		return x - y;
	case'*':
		return x*y;
	case'/':
		return x / y;
	}
	return 0;
}

int main(){
 mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lispy    = mpc_new("lispy");

  /* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \
      number   : /-?[0-9]+/ ;                             \
      operator : '+' | '-' | '*' | '/' ;                  \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      lispy    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Operator, Expr, Lispy);

  puts("Lispy Version 0.0.0.0.2");
  puts("Press Ctrl+c to Exit\n");

  while (1) {
    char* input = readline("lispy> ");
 

    /* Attempt to parse the user input */
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      /* On success print and delete the AST */
      mpc_ast_print(r.output);
	  
	  long x;
	  printf("%d \n",x=eval(r.output));
	  
      mpc_ast_delete(r.output);
    } else {
      /* Otherwise print and delete the Error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expr, Lispy);

  return 0;
}
