#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

//parser adjective to recognize descriptions
mpc_parser_t* Adjective = mpc_or(4, mpc_sym("wow"), mpc_sym("many"), mpc_sym("so"), mpc_sym("such"));

//build a parser noun to recognize things
mpc_parser_t* Noun = mpc_or(5, mpc_sym("lisp", mpc_sym("language"), mpc_sym("book"), mpc_sym("build"), mpc_sym("c"));

mpc_parser_t* Phrase = mpc_and(2, mpc_strfold, Adjective, Noun, free);

mpc_parser_t* Doge = mpc_many(mpcf_strfold, Phrase);

//interactive prompt program

static char input[2048];

int main(int argc, char**argv){
/* Print Version and Exit Information */
	puts("Lispy Version 0.0.0.0.1");
	puts("Press Ctrl+c to Exit\n");

while(1){
	//output prompt
	char* input = readline("talktome> ");
	
	//add input to history
	add_history(input);
	
	//repeat input
	printf("yo mamma so thick she a %s", input);	
	
	//free input
	free(input);
	}

	return 0;
}
