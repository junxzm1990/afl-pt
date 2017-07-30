#include<stdio.h>
typedef void (*f1_ty)();
typedef int (*f2_ty)(int, int, int);
f2_ty f2;
f1_ty f1;

void ind_call(f2_ty f2, int loop){
    int i = 0;
    int d=loop-30, e=loop-2, f=loop%3;
    for(;i<loop;i++){
        d = i+d;
        /* if(d % 2 == 0){ */
        /*     e = i -2+e; */
        /*     if(e %3 ==0){ */
        /*         f = 0; */
        /*     } */
        /*     f = i+2+f; */
        /* } */
    }
    /* f2(e, f, d); */
}


int another_ind_call(int a, int b, int c){
    if(c%2 ==0){
        return a+b;
    }
    return a-b;
}

void main(){
    int d;
    f1 = &ind_call;
    f2 = &another_ind_call;
    scanf("%d",&d);
    if ( d%3 == 0)
        f1(f2, d);
}
