#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static int cmd_p(char *args);

static int cmd_w(char *args);

static int cmd_d(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
        { "si", "Single-step execution",cmd_si },
        { "info", "Print status of register or watchpoint", cmd_info },
        { "x", "Scanning memory", cmd_x },
        { "p", "Evaluate expression", cmd_p },
        { "w", "Set watchpoint", cmd_w},
        { "d", "Delete watchpoint", cmd_d},

	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

static int cmd_si(char *args){
        char *arg = strtok(args," ");
        if(arg == NULL){
                cpu_exec(1);
                return 0;
        }
        int times;
        sscanf(arg,"%d",&times);
        int i;
        for(i = 0;i < times;i++)
        cpu_exec(1);
        return 0;
};

static int cmd_info(char *args){
        char *arg = strtok(args," ");
        //cpu status
        if(strcmp(arg,"r") == 0){
                printf("eax is %x\n",cpu.eax);
                printf("ecx is %x\n",cpu.ecx);
                printf("edx is %x\n",cpu.edx);
                printf("ebx is %x\n",cpu.ebx);
                printf("esp is %x\n",cpu.esp);
                printf("ebp is %x\n",cpu.ebp);
                printf("esi is %x\n",cpu.esi);
                printf("edi is %x\n",cpu.edi);
        }  
        else if(args[0] == 'w')
                info_wp();
        return 0;
}

static int cmd_x(char *args){
        if(args == NULL){
                printf("too few parameter!\n");
                return 1;
        }
        
        char *arg = strtok(args," ");     
        if(arg == NULL){
                printf("too few parameter!\n");
                return 1;
        }
        int times = atoi(arg);
        char *str_E = strtok(NULL," ");
        if(str_E == NULL){
                printf("too few parameter!\n");
                return 1;
        } 
        if(strtok(NULL," ") != NULL){
                printf("too many parameter!\n");
                return 1;
        } 
        bool flag = true;
        //swaddr_t addr = expr(str_E , &flag);
        if(flag != true){
                printf("ERROR!\n");
                return 1;
        }
        char *str;
        //swaddr_t addr = atoi(str_E);
        swaddr_t addr = strtol(str_E,&str,16);
        //Scan 4 bytes every time when scanning memory
        int i,j;
        for(i = 0;i < times;i++){
                uint32_t data = swaddr_read(addr + i*4,4);
                printf("0x%08x ",addr + i*4);
                for(j = 0;j < 4;j++){
                        printf("0x%02x ",data & 0xff);
                        data = data >> 8;
                }
                printf("\n");
        }
        return 0;
}

static int cmd_p(char *args){
        int ans;
        bool success;
        ans = expr(args, &success);
        if(success)
                printf("0x%x:\t%d\n", ans, ans);
        else
                printf("wrong!");
        return 0;
}

static int cmd_w(char *args){
        WP *p;
        bool suc;
        p = new_wp();
        printf("Watchpoint %d: %s\n", p->NO, args);
        p->val = expr(args, &suc);
        strcpy(p->expr, args);
        if(!suc)          Assert(1,"wrong!\n");
        printf("Value:%d\n", p->val);
        return 0;
}

static int cmd_d(char *args){
        int n;
        sscanf(args, "%d", &n);
        delete_wp(n);
        return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
