//
// Created by Wenxin Zheng on 2021/4/21.
//

#ifndef ACMOS_SPR21_ANSWER_PRINTK_H
#define ACMOS_SPR21_ANSWER_PRINTK_H
static void printk_write_string(const char *str) {
    // Homework 1: YOUR CODE HERE
    // this function print string by the const char pointer
    // I think 3 lines of codes are enough, do you think so?
    // It's easy for you, right?

    while(*str != '\0'){
        uart_putc(*str);
        str++;
    }

}


static void printk_write_num(int base, unsigned long long n, int neg) {
    // Homework 1: YOUR CODE HERE
    // this function print number `n` in the base of `base` (base > 1)
    // you may need to call `printk_write_string`
    // you do not need to print prefix like "0x", "0"...
    // Remember the most significant digit is printed first.
    // It's easy for you, right?
    int k=33;
    char s[34];
    s[k]='\0';
    if(neg==1)
        s[--k]='-';
    if(n==0)
        s[--k]='0';
    if(base==2) {
        while(n){
            s[--k]='0'+n%2;
            n/=2;
        }
    }
    else if(base==10) {
        while(n){
            s[--k]='0'+n%10;
            n/=10;
        }
    }
    else if(base==16){
        while(n){
            if(n%16<10)
                s[--k]='0'+n%16;
            else
                s[--k]='a'+n%16-10;
            n/=16;
        }
    }
    printk_write_string(&s[k]);
}

#endif  // ACMOS_SPR21_ANSWER_PRINTK_H
