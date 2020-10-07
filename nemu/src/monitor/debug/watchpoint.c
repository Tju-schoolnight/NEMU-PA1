#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include "nemu.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp(){
        WP *p, *f;
        p = head;
        f = free_;
        free_ = free_->next;
        f->next = NULL;
        if(p == NULL){
                head = f;
                p = head;
        }
        else{
                while(p->next != NULL)
                       p = p->next;
                p->next = f;
        }
        return f;
}

void free_wp(WP *wp){
        WP *p, *f;
        f = free_;
        if(f == NULL){
                free_ = wp;
                f = free_;
        } 
        else{
                while(f->next != NULL)
                       f = f->next;
                f->next = wp;
        }
        p = head;
        if(head == NULL)            assert(0);
        if(head->NO == wp->NO)
                head = head->next;
        else{
                while(p->next != NULL && p->next->NO != wp->NO)
                             p = p->next;
                if(p->next == NULL && p->NO == wp->NO)
                             printf("wrong!\n");
                else if(p->next->NO == wp->NO)
                             p->next = p->next->next;
                else         assert(0);
        }
        wp->next = NULL;
        wp->val = 0;
        wp->b = 0;
        wp->expr[0] = '\0';
}

bool check_wp(){
        WP* p;
        p = head;
        bool fl = true;
        bool success;
        while(p != NULL){
                 uint32_t texpr = expr(p->expr, &success);
                 if(!success)         assert(1);
                 if(texpr != p->val){
                             fl = false;
                             if(p->b){
                                      printf("Hit breakpoint %d at 0x%08x\n", p->b, cpu.eip);
                                      p = p->next;
                                      continue;
                             } 
                             printf("Watchpoint %d: %s\n", p->NO, p->expr);
                             printf("Old value = %d\n", p->val);
                             printf("New value = %d\n", texpr);
                             p->val = texpr;
                 }
                 p = p->next;
        }
        return fl;
}

void delete_wp(int n){
        WP *p;
        p = &wp_pool[n];
        free_wp(p);
}

void info_wp(){
        WP *p;
        p = head;
        if(p == NULL)           printf("no watchpoint!\n");
        while(p != NULL){
                 printf("Watchpoint %d: %s = %d\n", p->NO, p->expr, p->val);  
                 p = p->next;
        }
}
