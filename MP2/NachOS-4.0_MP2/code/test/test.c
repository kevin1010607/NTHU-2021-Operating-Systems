#include "syscall.h"
int main(void){
	int a = Open("file1.test");
	int b = Open("file1.test");
	PrintInt(a);
	PrintInt(b);
	if(a != b) MSG("Fail\n");
	else MSG("Success\n");
	Halt();		
}
