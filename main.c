#include <stdio.h>
#include "mpc.h"

enum{LVAL_NUM,LVAL_ERR,LVAL_SYM,LVAL_SEXPR};


typedef struct lval{
	
	int type;
	long num;
	
	char *sym;
	char *err;
	
	 int count;
	 struct lval ** cell;
}lval;



char * readline(char * read) {
	fputs(read,stdout);
	char *in=(char *)malloc(sizeof(char)*50);
	gets(in);
	return in ;
	
}



lval* lval_num(long x) {
  lval *v=malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval* lval_err(char * er){
	lval *v=malloc(sizeof(lval));
	v->type=LVAL_ERR;
	v->err=malloc(strlen(er)+1);
	strcpy(v->err,er);
	return v;
}

lval* lval_sym(char * a){
  lval *v=malloc(sizeof(lval));
  v->type=LVAL_SYM;
  v->sym=malloc(strlen(a)+1);
  strcpy(v->sym,a);
  return v;
}

lval* lval_sexpr(){
  lval *v=malloc(sizeof(lval));
  v->type=LVAL_SEXPR;
  v->count=0;
  v->cell=NULL;
  return v;
}



lval * lval_add(lval *x,lval *y){
  (x->count)++;
  x->cell=realloc(x->cell,sizeof(lval*)* x->count);
  x->cell[x->count-1]=y;
  return x;
}

void lval_del(lval *v){
  switch (v->type) {
    /* Do nothing special for number type */
    case LVAL_NUM: break;

    /* For Err or Sym free the string data */
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;

    /* If Sexpr then delete all elements inside */
    case LVAL_SEXPR:
      for (int i = 0; i < v->count; i++) {
        lval_del(v->cell[i]);
      }
      /* Also free the memory allocated to contain the pointers */
      free(v->cell);
    break;
  }

  /* Free the memory allocated for the "lval" struct itself */
  free(v);
}

lval* lval_read_num(mpc_ast_t *t){
  long d;
  errno=0;
  d=strtol(t->contents,NULL,10);
  return errno!=ERANGE
	? lval_num(d)
	: lval_err(" invalid number");
	
}

lval * lval_pop(lval* a,int i){
  lval * x=a->cell[i];
  memmove(&a->cell[i],&a->cell[i+1],sizeof(lval *) *(a->count-i-1));
  a->count--;
  a->cell=realloc(a->cell,sizeof(lval )*a->count);
  return x;
}
lval * lval_take(lval *a ,int i){
  lval *x=lval_pop(a,i);
  lval_del(a);
  return x;
}


lval * lval_read(mpc_ast_t* t){
  if(strstr(t->tag,"number")){
    return lval_read_num(t);
  }
  if(strstr(t->tag,"symbol"))
    return lval_sym(t->contents);

  lval * x=NULL;
  
  if((strcmp(t->tag,">")==0)||strstr(t->tag,"sexpr"))
	x=lval_sexpr();

  int i;
  for(i=0;i<t->children_num ;i++){
    if(strcmp(t->children[i]->contents,"(")==0) continue;
	if(strcmp(t->children[i]->contents,")")==0) continue;
	if(strcmp(t->children[i]->contents,"{")==0) continue;
	if(strcmp(t->children[i]->contents,"}")==0) continue;
	if(strcmp(t->children[i]->tag,"regex")==0) continue;
	x=lval_add(x,lval_read(t->children[i]));
  }
  return x;
}


lval* eval_op(lval *x, char* op) {

  /* If either value is an error return it */
  //if (x == NULL) { return x; }
  int i;
  for(i=0;i<x->count;i++)
	if(x->cell[i]->type!=LVAL_NUM) return lval_err("con't operate on non-number!");


  
  lval *a=lval_pop(x,0);
 // if(x->count==0) return a;
  if(x->count==0&&*op=='-') return lval_num(-(a->num));
  
  while(x->count!=0){
	  lval *y=lval_pop(x,0);
  /* Otherwise do maths on the number values */
  if (strcmp(op, "+") == 0) { a->num=a->num+y->num; }
  if (strcmp(op, "-") == 0) { a->num=a->num-y->num; }
  if (strcmp(op, "*") == 0) { a->num=a->num*y->num; }
  if (strcmp(op, "/") == 0) {
    /* If second operand is zero return error */
    if(y->num=0){
	  lval_del(a);
	  lval_del(y);
	  a=lval_err("error:div 0");
	}
	else a->num=a->num/y->num; 
	//printf("%d",a->num);
  }
   lval_del(y);
  
  }
  return a;
}

/*
lval eval(mpc_ast_t* t) {

  if (strstr(t->tag, "number")) {
    
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  char* op = t->children[1]->contents;  
  lval x = eval(t->children[2]);

  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;  
}

*/


lval* lval_eval(lval * a);
lval * lval_eval_sexpr(lval * v){
  int i;
  for(i=0;i<v->count;i++)
	v->cell[i]=lval_eval(v->cell[i]);

  if(v->count==0) return v;
  for(i=0;i<v->count;i++){
    if(v->cell[i]->type==LVAL_ERR) return lval_take(v,i);
	
  }
  if(v->count==1) return lval_take(v,0);
  
  
  lval * op=lval_pop(v,0);
  if(op->type!=LVAL_SYM) {
    lval_del(op);
	lval_del(v);
	return lval_err("no sym");
  }
  
  lval *result =eval_op(v,op->sym);
 // printf("%d",result->num);
  lval_del(op);
  return result;
}

lval * lval_eval(lval *x){
  if(x->type==LVAL_SEXPR) return lval_eval_sexpr(x);
  return x;
}


void lval_expr_print(lval* v,char open,char close);
void lval_print(lval *a){	
	switch(a->type){
		case LVAL_NUM: 
			printf("%li",a->num);
			break;
		case LVAL_SYM: 
			printf("%s",a->sym);
			break;
		case LVAL_ERR:
			printf("Error: %s",a->err);
			break;
		case LVAL_SEXPR:
			lval_expr_print(a,'(',')');
			break;
		}
	
}

void lval_expr_print(lval* v,char open,char close){
  putchar(open);
  int i;
  for(i=0;i<v->count;i++){
    lval_print(v->cell[i]);
	
	if(i!=(v->count-1))
		putchar(' ');
  }
  
  putchar(close);
}

void lval_println(lval *a){
  lval_print(a);
  printf("\n");
}


int main(){
   mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr  = mpc_new("sexpr");
  mpc_parser_t* Expr   = mpc_new("expr");
  mpc_parser_t* Lispy  = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT,
    "                                          \
      number : /-?[0-9]+/ ;                    \
      symbol : '+' | '-' | '*' | '/' ;         \
      sexpr  : '(' <expr>* ')' ;               \
      expr   : <number> | <symbol> | <sexpr> ; \
      lispy  : /^/ <expr>* /$/ ;               \
    ",
    Number, Symbol, Sexpr, Expr, Lispy);

  puts("Lispy Version 0.0.0.0.5");
  puts("Press Ctrl+c to Exit\n");
  while (1) {
    char* input = readline("lispy> ");
 

    /* Attempt to parse the user input */
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      /* On success print and delete the AST */
      mpc_ast_print(r.output);
	  
	lval* x = lval_read(r.output);
	lval_println(x);
	lval *tt=lval_eval(x);
	lval_println(tt);
	lval_del(x);
	  
      mpc_ast_delete(r.output);
    } else {
      /* Otherwise print and delete the Error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number,Symbol, Expr, Lispy,Sexpr);

  return 0;
}
