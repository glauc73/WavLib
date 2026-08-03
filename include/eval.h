#ifndef _EVAL_H_
#define _EVAL_H_
#include <ctype.h>
#include <stdbool.h>

#ifndef dx
#define dx 1e-6
#endif

#ifndef SIZE_ARR
#define SIZE_ARR(X) sizeof(X) / sizeof(X[0])
#endif

#ifndef NUM_FUNCTIONS 
#define NUM_FUNCTIONS 14
#endif

#ifndef NUM_VALID_OPERATE
#define NUM_VALID_OPERATE 10
#endif

#define BUFSIZE 512

//tipo de expressões matematicas, guardando a expressão (input), o vetor com as raizes, a quantidade de raizes, o resultado
//o status, a variavel (signed char pq MULTIPLY_VAR = -1, e os ponteiros pro lado direito e esquerdo)

int isoperat(char ch);
void removespace(char* expr);
int balanced_brackets(char* expr);
char invalid_char(char *iterator);
void add_product(char *exp);
char checkvar(char *exp);

void strlower(char *str);
int lenFunc(char *exp);
double PI_(char **exp);
double euller_number(char **exp);
double parse_abs(char **exp);
double parse_sqrt(char **exp); 
double parse_cbrt(char **exp); 
double parse_tan(char **exp);
double parse_sin(char **exp);
double parse_cos(char **exp);
double parse_arctan(char **exp);
double parse_arcsin(char **exp);
double parse_arccos(char **exp);
double parse_ln(char **exp);
double parse_log(char **exp);
double parse_bracket(char **exp);
double parse_func(char **exp);
double parse_pow(char **exp);
double parse_factor(char **exp);
double math_parser(char **exp);
double eval_X(char *exp, double X);
double eval(char *exp);
double trapz(char* expr, double a, double b, int n);

typedef struct{
	const char *name;
	double (*call)(char **);
} func_parser;

typedef enum
{
	MULTIPLY_VAR = -1,
	NOT_VAR,
} var_err;

extern const func_parser functions[];
extern const char valid_operate[];

#endif
