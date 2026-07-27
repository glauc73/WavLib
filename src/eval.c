#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "eval.h"
#include <complex.h>

#define _USE_MATH_DEFINES
signed char var = 'x';

const char valid_operate[] = {'+', '-', '*', '/', '(', ')', '^', '=', '.', ','};
//verifica se é um dos operadores validos
int isoperat(char ch)
{
	for (int i = 0; i < SIZE_ARR(valid_operate); i++){
		if(ch == valid_operate[i])   //procura o caracter mencionado no array de operadores validos
			return 1;
	}
	return 0;
}

char invalid_char(char *iterator)
{
	while (*iterator)
	{
		if (ispunct(*iterator) && !isoperat(*iterator) ){ //verifica se possui um operador invalido (simbolo que nao está em valid operate)
			return *iterator;  //retorna esse caracter se encontrar
		}
		iterator++;
	}
	return 0;
}

int balanced_brackets(char* expr) {
	int balance = 0;
	while(*expr){
		if(*expr == '(') balance++;
		else if (*expr == ')') balance--;

		if(balance < 0) return 0;
		expr++;
	}
	return (balance == 0);
}

char checkvar(char *expr)
{
	int count = 0;
	char varx = 0, atual_var = 0;
	
	while (*expr)
	{	int len_func = lenFunc(expr);
		if(len_func) {  //ignora os caracteres das funções, para não confundir com variaveis
			expr +=len_func;
			continue;
		}
		if(!isalpha(*expr)) { //se não for uma letra, ignora e retoma o loop
			expr++;
			continue;
		}
		atual_var = *expr;

		if(count == 0) {
			varx = atual_var;
		}else if(atual_var != varx){
			return MULTIPLY_VAR;
		}
		count++;
		//a cada caracter novo encontrado, adiciona no array e compara com var[0]
		expr++; //caso o ponteiro atual aponte uma função, exp a pula, se não, pula penas 1;
	}

	return count == 0 ? NOT_VAR : varx;
}

void add_product_before_func(char *input)
{	
    if (input == NULL || *input == '\0') return;

    // Aloca memória com espaço seguro para os asteriscos adicionais
    char* func = calloc(6 * strlen(input) + 1, sizeof(char));
    if (func == NULL) return;

    char* aux = input;
    while (*input)
    {	
        int len_func = lenFunc(input);

        if (len_func > 0) {  
            strncat(func, input, len_func);
            input += len_func;
            continue;
        }

        if (*input == ',') *input = '.'; 
        
        // Copia o caractere atual para a nova string
        strncat(func, input, 1); 

        char atual = *input;
        char *next_ptr = input + 1;

        // Verifica se o PRÓXIMO caractere inicia uma função (espiando a frente)
        int next_is_function = (lenFunc(next_ptr) > 0);

        // Regra estrita: Só adiciona '*' se o atual for número/parêntese e o próximo for função
        int implicit_product = next_is_function && (isdigit(atual) );

        if (implicit_product) {
            strcat(func, "*");
        }
        
        input++;
    }
    
    strcpy(aux, func); 
    free(func); 
}

void add_product(char *input)
{	
	char* func = calloc(6 * strlen(input), sizeof(char));
	char* aux = input;
	while (*input)
	{	
		int len_func = lenFunc(input);

		if(len_func){  //antes de tudo verifica se o ponteiro aponta uma função, para copiá- la de uma vez. permitindo que xln(x) vire x*ln(x), por exemplo
			strncat(func, input, len_func);
			input += len_func;
			continue;
		}
		if(*input == ',') *input = '.'; //adiciona a operação de ',
		strncat(func, input, 1); //copia caracter a caracter input para func

		char atual = *input;
		char next = *(input + 1);
		//nos seguintes casos o usuario usou uma multiplicação implicita. o programa adiciona '*' entre os caracteres para que eval() possa interpretar.
		
		int implicit_product = 
			(isdigit(atual) && isalpha(next)) 	|| 	// 2x -> 2*x
			((atual == ')' && next == '(') )	||	// )( -> )*(
			(isdigit(atual) && next == '(')		|| 	// 2( -> 2*(
			(atual == ')' && isdigit(next))		|| 	// )2 -> )*2
			(isalpha(atual) && next == '(') 	|| 	// x( -> x*(
			(atual == ')' && isalpha(next))		||	// )x -> )*x
			(isalpha(atual) && isalpha(next));		// xy -> x*y

		if (implicit_product){
			strcat(func, "*");
		}
		input++;
	}
	strcpy(aux, func); 
	free(func); //copia func para o ponteiro inicial de input, que está em aux 
}


const func_parser functions[] = // mapa contendo as funcoes e suas respectivas strings
{ 
	{"tan(", parse_tan},
	{"sin(", parse_sin},
	{"cos(", parse_cos},
	{"arctan(", parse_arctan},
	{"arcsin(", parse_arcsin},
	{"arccos(", parse_arccos},
	{"log(", parse_log},
	{"ln(", parse_ln},
	{"sqrt(", parse_sqrt},
	{"cbrt(", parse_cbrt},
	{"abs(", parse_abs},
	{"e", euller_number},
	{"pi", PI_}, 
	{"(", parse_bracket},
};

void strlower(char *str){
	while (*str){
		if (*str >= 'A' && *str <= 'Z')
			*str += 'a' - 'A';  //soma a diferença entre maiusculo e minusculo na tabela ascii, convertendo
		str++;
	}
}

//verifica se o ponteiro ata=ual é uma função definida e retorna seu tamanho
//retorna 0 se não for uma função
int lenFunc(char *expr){
	for (size_t i = 0; i < SIZE_ARR(functions); i++){
		if (strncmp(expr, functions[i].name, strlen(functions[i].name)) == 0)  //compara se a posição atual do ponteiro tem o nome de uma função
			return strlen(functions[i].name); //se tiver, retorna o tamanho da função
	}
	return 0;
}

//é a ultima chamada da função, e retorna o numero na qual o ponteiro aponta
double parse_number(char **expr){
	char *endptr;
	double result = strtod(*expr, &endptr);
	*expr = endptr;   //a exprressão recebe o ponteiro para o final do numero, encerrando a função/ recursão e começando a retornar
	return result;
}

double parse_bracket(char **expr) {
	*expr += strlen("(");
	//ao encontrar um parenteses, a função chama a si mesma de forma recursiva, pois a exprressão dentro dele independe do resto.
	double result = math_parser(expr); 
	//esse raciocinio se segue para as demais funções
	(*expr)++;
	return result;
}

double parse_abs(char **expr) {
	*expr += strlen("abs(");
	double result = fabs(math_parser(expr));
	(*expr)++;
	return result;
}

double parse_sqrt(char **expr) {
	*expr += strlen("sqrt(");
	double result = sqrt(math_parser(expr));
	(*expr)++;
	return result;
}

double parse_cbrt(char **expr) {
	*expr += strlen("cbrt(");
	double result = cbrt(math_parser(expr));
	(*expr)++;
	return result;
}

double parse_tan(char **expr)
{
	*expr += strlen("tan(");
	double result = tan(math_parser(expr));
	(*expr)++;
	return result;
}

double parse_sin(char **expr)
{
	*expr += strlen("sin(");
	double result = sin(math_parser(expr));
	(*expr)++;
	return result;
}

double parse_cos(char **expr)
{
	*expr += strlen("cos(");
	double result = cos(math_parser(expr));
	(*expr)++;
	return result;
}

double parse_arcsin(char **expr) {
	*expr += strlen("arcsin(");
	double result = asin(math_parser(expr));
	(*expr)++;
	return result;
}

double parse_arccos(char **expr) {
	*expr += strlen("arccos(");
	double result = acos(math_parser(expr));
	(*expr)++;
	return result;
}

double parse_arctan(char **expr) {
	*expr += strlen("arctan(");
	double result = atan(math_parser(expr));
	(*expr)++;
	return result;
}

double parse_log(char **expr)
{
	*expr += strlen("log(");
	double result = log10(math_parser(expr));
	(*expr)++;
	return result;
}

double parse_ln(char **expr)
{
	*expr += strlen("ln(");
	double result = log(math_parser(expr));
	(*expr)++;
	return result;
}

double euller_number(char **expr)
{
	*expr += strlen("e");
	return M_E;
}

double PI_(char **expr)
{
	*expr += strlen("pi");
	return M_PI;
}

double parse_func(char **expr){
	// checa se a posicao atual do ponteiro é uma das funcoes mapeadas (sen, cos, log, etc)
	for (size_t i = 0; i < sizeof(functions) / sizeof(func_parser); i++)
	{
		if (strncmp(*expr, functions[i].name, strlen(functions[i].name)) == 0)
		{
			return functions[i].call(expr);  //se for alguma das funções, ele a retorna;
		}
	}
	// default : recebe o numero
	return parse_number(expr);
}

double parse_pow(char **expr){
	double factor = parse_func(expr);
	double result = factor;
	while (**expr == '^'){
		while (**expr == '^')  //skipa no caso de o usuario digitar '^^^^^^'
			(*expr)++;

		double nextfactor = parse_func(expr);
		result = pow(result, nextfactor);	
	}
	return result;
}

double parse_factor(char **expr){
	double factor = parse_pow(expr);
	double result = factor;
	while (**expr == '*' || **expr == '/'){
		char op = **expr;
		while(**expr == '*' || **expr == '/') {
			(*expr)++;
		}
		 // lembre-se de incrementar o ponteiro para nao analisar de novo o sinal e dar erro no retorno
		double nextfactor = parse_pow(expr);
		result *= (op == '*') ? nextfactor : 1 / nextfactor;
	}
	return result;
}

double math_parser(char **expr){
	double term = parse_factor(expr);
	double result = term;
	while (**expr == '+' || **expr == '-')
	{
		char op = **expr;
		while (**expr == '+' || **expr == '-'){
			(*expr)++;
		}
		
		double nextterm = parse_factor(expr);
		result += (op == '+') ? nextterm : -nextterm;
	}
	
	return result;
}

void remove_spaces(char* expr){
    if(expr == NULL) return;
    int dest_idx = 0;
    for(int i = 0; expr[i]; i++){
        if(expr[i] != ' ') expr[dest_idx++] = expr[i];
    }
    expr[dest_idx] = '\0';
}

void clean_expr(char* expr){
    strlower(expr);
    remove_spaces(expr);
}

double eval(char *expr){
    if(invalid_char(expr) || !balanced_brackets(expr) || checkvar(expr) != 0) return NAN;
    char *buffer = calloc(6*(strlen(expr) + 1), sizeof(char));
    strcpy(buffer, expr);
    add_product_before_func(buffer);

	clean_expr(buffer);
    char* begin_buffer = buffer;
	double result = math_parser(&buffer);
    free(begin_buffer);
    return result;
}

double eval_X(char *expr, double X){
    var = checkvar(expr);
    if(invalid_char(expr) || !balanced_brackets(expr) || var == MULTIPLY_VAR) return NAN;
    if(var == 0) return eval(expr);
    char *buffer = calloc(6*(strlen(expr) + 1), sizeof(char));
    char* begin = buffer;
    strcpy(buffer, expr);

    clean_expr(buffer);
    add_product(buffer);
	// array que vai guardar a nova funcao com o conteudo de X
	char* func_x = calloc(10 * strlen(buffer), sizeof(char));
	while (*buffer)
	{
		int len_func = lenFunc(buffer);

		if(len_func) {  //verifica as funções antes, porque o usuartio pode escolher uma variavel que a compoe. ex: s*sin(pi/6) = 2
			strncat(func_x, buffer, len_func);
			buffer += len_func;
			continue;
		}
		if (*buffer == var){
			char value[32];
			sprintf(value, "(%.10lf)", X);
			strcat(func_x, value);  ///coloca o numero entre parenteses na exprressão, para respeitar a ordem de precedencia
		}else {
			strncat(func_x, buffer, 1);
		}
		buffer++;
	}
	char *ptr = func_x;
	double result = math_parser(&ptr);
	free(func_x);
    free(begin);

	return result;
}

double trapz(char* f, double a, double b, int n){
    double h = (b - a)/n;
    double s = 0;
    for(int i = 1; i < n; i++){
        double xi = a + h*i;
        double yi = eval_X(f, xi);
        if(yi == yi) s += yi;
    }
    double y0 = eval_X(f, a);
    double yn = eval_X(f, b);
    
    y0 = y0 == y0 ? y0 : 0.0;
    yn = yn == yn ? yn : 0.0;

    s = (y0 + yn)*h/2 + h*s;
    return s;
}
