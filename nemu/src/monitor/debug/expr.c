#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ,NUMBER,NEQ,AND,OR,POINTER,HEX,REGISTER,MINUS                 

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
        int priority;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE, 0},				   // spaces
	{"\\+", '+', 4},				   // plus
        {"\\-", '-', 4},                                   // sub
        {"\\*", '*', 5},                                   // multiply
        {"/", '/', 5},                                     // divide
	{"==", EQ, 3},                             	   // equal
        {"\\b[0-9]+\\b", NUMBER, 0},                       // number
        {"\\(", '(', 7},                                   // left bracket
        {"\\)", ')', 7},                                   // right bracket
        {"!", '!', 6},                                     // not
        {"!=", NEQ, 3},                                    // not equal
        {"&&", AND, 2},                                    // and
        {"\\|\\|", OR, 1},                                 // or
        {"\\b0[xX][0-9a-fA-F]+\\b", HEX, 0},               // hexnumber
        {"\\$[a-zA-Z]+", REGISTER, 0},                     // register
                                                        
         
                                                                                       //wait for tabs
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
        int priority;
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
                                char *fol = e + position + 1;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
                                        case NOTYPE: break;
                                        case REGISTER:
                                                 tokens[nr_token].type = rules[i].token_type;
                                                 tokens[nr_token].priority = rules[i].priority;
                                                 strncpy(tokens[nr_token].str,fol,substr_len-1);
                                                 tokens[nr_token].str[substr_len-1] = '\0';
                                                 nr_token++;
                                                 break;
					default: 
                                                 tokens[nr_token].type = rules[i].token_type;
                                                 tokens[nr_token].priority = rules[i].priority;
                                                 strncpy(tokens[nr_token].str,substr_start,substr_len);
                                                 tokens[nr_token].str[substr_len] = '\0';
                                                 nr_token++;
				}
                                position += substr_len;
				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}

bool check_parentheses(int l, int r){
        int i;
        if(tokens[l].type == '(' && tokens[r].type == ')'){
               int lnum = 0, rnum = 0;

               for(i = l+1; i<r; i++){
                        if(tokens[i].type == '(')               lnum++;
                        if(tokens[i].type == ')')               rnum++;
                        if(rnum > lnum)                         return false;           
               }      
 
               if(lnum == rnum)                                 return true;
        }  
        return false;
}

int dominant_operator(int l,int r){
        int i,j;
        int Dop = l,pri_min = 10;
        for(i = l;i <= r;i++){
                if(tokens[i].type == NUMBER || tokens[i].type == HEX || tokens[i].type == REGISTER)
                          continue;
                int sum = 0;
                bool flag = true;
                for(j = i-1; j >= l; j--){
                          if(tokens[j].type == '(' && !sum){
                                     flag = false;
                                     break;
                          }
                          if(tokens[j].type == '(')            sum--;
                          if(tokens[j].type == ')')            sum++;
                }
                if(!flag)              continue;
                if(tokens[i].priority <= pri_min){
                           pri_min = tokens[i].priority;
                           Dop = i;
                }
        }
        return Dop;
}

int eval(int l,int r){
        if(l > r){
                Assert(l > r, "something happened!\n");
                return 0;
        }
        if(l == r){
                int num = 0;
                if(tokens[l].type == NUMBER)                    sscanf(tokens[l].str,"%d",&num);
                if(tokens[l].type == HEX)                       sscanf(tokens[l].str,"%x",&num);
                if(tokens[l].type == REGISTER){
                         if(strlen(tokens[l].str) == 3){
                                     int i;
                                     for(i = R_EAX; i <= R_EDI;i++)
                                               if(strcmp(tokens[l].str, regsl[i]) == 0)            break;
                                               if(i > R_EDI)
                                               if(strcmp(tokens[l].str, "eip") == 0)
                                                       num = cpu.eip;
                                               else Assert(1, "wrong register!\n");
                                     else num = reg_l(i);                                 
                         } 
                         else if(strlen(tokens[l].str) == 2){
                                     if(tokens[l].str[1] == 'x' || tokens[l].str[1] == 'p' || tokens[l].str[1] == 'i'){
                                               int i;
                                               for(i = R_AX; i <= R_DI; i++)
                                                       if(strcmp(tokens[l].str, regsw[i]) == 0)      break;
                                               num = reg_w(i);
                                     }
                                     else if(tokens[l].str[1] == 'l' || tokens[l].str[1] == 'h'){
                                               int i;
                                               for(i = R_AL; i <= R_BH; i++)
                                                       if(strcmp(tokens[l].str, regsb[i]) == 0)      break;
                                               num = reg_b(i);
                                     } 
                                     else
                                               Assert(1, "wrong register!\n");
                         }
                }
                return num;
        }
        else if(check_parentheses(l,r) == true)               return eval(l+1, r-1);
        else{
                int D_op = dominant_operator(l,r);
                if(l == D_op || tokens[D_op].type == POINTER || tokens[D_op].type == MINUS || tokens[D_op].type == '!'){
                           int val = eval(l+1, r);
                           switch(tokens[l].type){
                                     case POINTER: current_sreg = R_DS;
                                                   return swaddr_read(val, 4);
                                     case MINUS: return -val;
                                     case '!' :  return !val;
                                     default :   Assert(1, "default\n");
                           }
                }
                int v1 = eval(l, D_op-1);
                int v2 = eval(D_op+1, r);
                switch(tokens[D_op].type){
                           case '+': return v1+v2;
                           case '-': return v1-v2;
                           case '*': return v1*v2;
                           case '/': return v1/v2;
                           case EQ:  return v1==v2;
                           case NEQ: return v1!= v2;
                           case AND: return v1&&v2;
                           case OR : return v1||v2;
                           default : break;
                }
        }
        Assert(1,"wrong input!\n");
        return -100;
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	/* TODO: Insert codes to evaluate the expression. */
	int i;
        for(i = 0; i < nr_token; i++){
                if(tokens[i].type == '*' && (i == 0 || (tokens[i-1].type != NUMBER && tokens[i-1].type != HEX && tokens[i-1].type != REGISTER && tokens[i-1].type != ')'))){
                          tokens[i].type = POINTER;
                          tokens[i].priority = 6;
                }
                if(tokens[i].type == '-' && (i == 0 || (tokens[i-1].type != NUMBER && tokens[i-1].type != HEX && tokens[i-1].type != REGISTER && tokens[i-1].type != ')'))){
                          tokens[i].type = MINUS;
                          tokens[i].priority = 6;
                }
        }
        *success = true;
	return eval(0, nr_token-1);
}

