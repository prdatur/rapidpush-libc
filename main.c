/* 
 * File:   main.c
 * Author: prdatur
 *
 * Created on 26. Januar 2013, 18:26
 */
#include "rapidpush.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	printf("Return code: %d\n", rapidpush_notify("YOU-API-Key", "Title test", "message test", 2, "c_test", "", ""));
    return 0;
}
