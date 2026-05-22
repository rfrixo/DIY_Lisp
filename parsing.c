#include "mpc.h"

#ifdef _WIN32

static char buffer[2048];

char* readline(char* prompt) {
 	fputs(prompt, stdout);
  	fgets(buffer, 2048, stdin);
  	char* cpy = malloc(strlen(buffer)+1);
  	strcpy(cpy, buffer);
  	cpy[strlen(cpy)-1] = '\0';
  	return cpy;
}

void add_history(char* unused) {}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

/* Create Enumeration of Possible lval Types */
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

/* Create Enumeration of Possible Error Types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* Declare New lval Struct */
typedef struct lval {
	int type;
	long num;
	//error and symbol types have string data
	char* err;
	char* sym;
	//count and pointer to a list of lval
	int count;
	struct lval** cell;
} lval;

/* construct a pointer to a new Number lval */
lval* lval_num(long x) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

/* construct a pointer to a new Error lval */
lval* lval_err(char* m) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(m) + 1);
	strcpy(v->err, m);
	return v;
}

/* The strlen function only returns the number of bytes in a string excluding the null terminator. This is why we need to add one, to ensure there is enough allocated space for it all! */

//construct a pointer to a new Symbol lval
lval* lval_sym(char* s) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s) + 1);
	strcpy(v->sym, s);
	return v;
}

//a pointer to a new empty Sexpr lval
lval* lval_sexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

//a pointer to an new empty Qexpr lval
lval* lval_qexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

void lval_del(lval* v) {

	switch (v->type) {
		//do nothing special for num type
		case LVAL_NUM: break;
		
		//for Err or Sym free the string data
		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;
		
		//if Qexpr or Sexpr then delete all elements inside
		case LVAL_QEXPR:
		case LVAL_SEXPR:
			for (int i = 0; i < v->count; i++) {
				lval_del(v->cell[i]);
			}
			//also free the memory allocated to contain the pointers
			free(v->cell);
		break;
	}
	
	//free the memory allocated for the "lval" struct itself
	free(v);
}

lval* lval_add(lval* v, lval* x) {
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1] = x;
	return v;
}

lval* lval_pop(lval* v, int i) {
	//find item at i
	lval* x = v->cell[i];
	
	//shift memory after the item at "i" over the top
	memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));
	
	//decrease the count of items in the list
	v->count--;
	
	//reallocate memory used
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	return x;
}

lval* lval_take(lval* v, int i) {
	lval* x = lval_pop(v, i);
	lval_del(v);
	return x;
}

void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close) {
	putchar(open);
	for (int i = 0; i< v->count; i++) {
		//print value contained within
		lval_print(v->cell[i]);
		
		//don't print trailing space if last element
		if(i != (v->count-1)) {
			putchar(' ');
		}
	}
	putchar(close);
}

//Print an lval
void lval_print(lval* v) {
	switch (v->type) {
		case LVAL_NUM:	printf("%li", v->num); break;
		case LVAL_ERR:	printf("Error: %s", v->err); break;
		case LVAL_SYM:	printf("%s", v->sym); break;
		case LVAL_SEXPR:lval_expr_print(v, '(', ')'); break;
		case LVAL_QEXPR:lval_expr_print(v, '{', '}'); break;
	}
}

/* Print an "lval" followed by a newline */
void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* lval_eval(lval* v);

#define LASSERT(args, cond, err) \
	if (!(cond)) { lval_del(args); return lval_err(err); }

lval* builtin_head(lval* a) {
	LASSERT(a, a->count == 1, "Function 'head' passed too many arguments!");
	LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'head' passed incorrect type!");
	LASSERT(a, a->cell[0]->count != 0, "Function 'head' passed {}!");
	
	//otherwise take first argument
	lval* v = lval_take(a, 0);
	
	//delete all elements that are not head and return
	while (v->count > 1) { lval_del(lval_pop(v, 1));}
	return v;
}

lval* builtin_tail(lval* a) {
	LASSERT(a, a->count == 1, "Function 'tail' passed too many arguments!");
	LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'tail' passed incorrect type!");
	LASSERT(a, a->cell[0]->count != 0, "Function 'tail' passed {}!");
	
	//take first argument
	lval* v = lval_take(a, 0);
	
	//delete first element and return
	lval_del(lval_pop(v, 0));
	return v;
}

lval* builtin_list(lval* a) {
	a->type = LVAL_QEXPR;
	return a;
}

lval* builtin_eval(lval* a) {
	LASSERT(a, a->count == 1, "Function 'eval' passed too many arguments!");
	LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'eval' passed incorrect type!");
	
	lval* x = lval_take(a, 0);
	x->type = LVAL_SEXPR;
	return lval_eval(x);
}

lval* lval_join(lval* x, lval* y) {
	//for each cell in 'y' add it to 'x'
	while (y->count) {
		x = lval_add(x, lval_pop(y, 0));
	}
	
	//delete the empty 'y' and return 'x'
	lval_del(y);
	return x;
}

lval* builtin_join(lval* a) {
	for (int i = 0; i < a->count; i++) {
		LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'join' passed incorrect type!");
	}
	
	lval* x = lval_pop(a, 0);
	
	while (a->count) {
		x = lval_join(x, lval_pop(a, 0));
	}
	
	lval_del(a);
	return x;
}
	

lval* builtin_op(lval* a, char* op) {
	//ensure all arguments are numbers
	for (int i = 0; i < a->count; i++) {
		if (a->cell[i]->type != LVAL_NUM) {
			lval_del(a);
			return lval_err("Cannot operate on non-number!");
		}
	}
	
	//pop the first element
	lval* x = lval_pop(a, 0);
	
	//if no arguments and sub perform unary negation
	if (((strcmp(op, "sub") == 0 || strcmp(op, "-") == 0) && a->count == 0)) {
		x->num = -x->num;
	}
	
	//while there are elements remaining
	while (a->count > 0) {
		//pop the next element
		lval* y = lval_pop(a, 0);
		
		if (strcmp(op, "add") == 0 || strcmp(op, "+") == 0) { x->num += y->num; }
		if (strcmp(op, "sub") == 0 || strcmp(op, "-") == 0) { x->num -= y->num; }
		if (strcmp(op, "mult") == 0 || strcmp(op, "*") == 0) { x->num *= y->num; }
		if (strcmp(op, "div") == 0 || strcmp(op, "/") == 0) { 
			if (y->num == 0) {
				lval_del(x); lval_del(y);
				x = lval_err("Division by Zero!"); break;
			}
			x->num /= y->num; 
		}
		//delete element now finished with
		lval_del(y);
	}
		
	lval_del(a); 
	return x;
}

lval* builtin(lval* a, char* func) {
	if (strcmp("list", func) == 0) { return builtin_list(a); }
	if (strcmp("head", func) == 0) { return builtin_list(a); }
	if (strcmp("tail", func) == 0) { return builtin_list(a); }
	if (strcmp("join", func) == 0) { return builtin_list(a); }
	if (strcmp("eval", func) == 0) { return builtin_list(a); }
	//|| strcmp(func, "add") == 0 || strcmp(func, "sub") == 0 || strcmp(func, "mult") == 0 || strcmp(func, "div") == 0
	if (strstr("+-/*", func) == 0) { return builtin_op(a, func); }
	lval_del(a);
	return lval_err("Unknown Function!");
}

lval* lval_eval_sexpr(lval* v) {
	printf("lval_sexpr\n");
	//evaluate children
	for (int i = 0; i < v->count; i++) {
		v->cell[i] = lval_eval(v->cell[i]);
	}
	
	//error checking
	for (int i = 0; i < v->count; i++) {
		if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
	}
	
	//empty expression
	if (v->count == 0) { return v; }
	
	//single expression
	if (v->count == 1) { return lval_take(v, 0); }
	
	//ensure first element is a symbol
	lval* f = lval_pop(v, 0);
	if (f->type != LVAL_SYM) {
		lval_del(f); lval_del(v);
		return lval_err("S-Expression does not start with symbol!");
	}
	
	//call builtin operator
	lval* result = builtin(v, f->sym);
	lval_del(f);
	return result;
}

lval* lval_eval(lval* v) {
	//evaluate sexpression
	if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v);}
	//all other lval types remain the same
	return v;
}

lval* lval_read_num(mpc_ast_t* t) {
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ?
		lval_num(x) : lval_err("invalid number");
}

lval* lval_read (mpc_ast_t* t) {
	//if Symbol or Number return conversion to that type
	if (strstr(t->tag, "number")) { return lval_read_num(t);}
	if (strstr(t->tag, "symbol")) { return lval_sym(t->contents);}
	
	//if root (>) or sexpr then create empty list
	lval* x = NULL;
	if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
	if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
	if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }
	
	//fill this list with any valid expression contained within
	for (int i = 0; i < t->children_num; i++) {
		if(strcmp(t->children[i]->contents, "(") == 0) { continue; }
		if(strcmp(t->children[i]->contents, ")") == 0) { continue; }
		if(strcmp(t->children[i]->contents, "{") == 0) { continue; }
		if(strcmp(t->children[i]->contents, "}") == 0) { continue; }
		if(strcmp(t->children[i]->tag, "regex") == 0) { continue; }
		x = lval_add(x, lval_read(t->children[i]));
	}	
	return x;
}

int main(int argc, char** argv) {
  
	// Create Some Parsers - these are just definitions, they are later described for mcp
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Symbol = mpc_new("symbol");
	mpc_parser_t* Sexpr = mpc_new("sexpr");
	mpc_parser_t* Qexpr = mpc_new("qexpr");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");
	
	/* Define them with the following Language */
	mpca_lang(MPCA_LANG_DEFAULT,
		"										\
			number:		/-?[0-9]+/ ;			\
			symbol:		\"eval\" | \"join\" | \"tail\" | \"head\" | \"list\" | \"min\" | \"max\" | \"add\" | \"sub\" | \"mult\" | \"div\" | '+' | '-' | '*' | '/';	\
			sexpr: 		'(' <expr>* ')' ;		\
			qexpr:		'{' <expr>* '}' ;		\
			expr: 		<number> | <symbol> | <sexpr> | <qexpr> ;\
			lispy: 		/^/ <expr>* /$/ ;		\
		",
		Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
  
	puts("Lispy Version 0.0.0.0.6");
	puts("Press Ctrl+c to Exit\n");
	
	while (1) {

		char* input = readline("lispy> ");
		add_history(input);

		/* Attempt to parse the user input */
		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
		  /* On success print and delete the AST */
		  lval* x = lval_eval(lval_read(r.output));
		  lval_println(x);
		  lval_del(x);
		  mpc_ast_delete(r.output);
		  
		} else {
		  /* Otherwise print and delete the Error */
		  mpc_err_print(r.error);
		  mpc_err_delete(r.error);
		}
		//printf("free(input)");
		free(input);
	}

	/* Undefine and delete our parsers */
	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

	return 0;
}
