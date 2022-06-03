#include <iostream>
#include "sim_mem.h"



    int main()
    {
        char val;
        sim_mem mem_sm((char*)"exec1.txt",(char*)"" ,(char*)"swap_file" ,25, 50, 25,25, 25, 5,1);

        mem_sm.store(1, 98,'X');
        val = mem_sm.load (1, 98);
        printf("%c\n",val);
        val = mem_sm.load (1, 20);
        printf("%c\n",val);
        mem_sm.store(1, 80,'Z');
        val = mem_sm.load (1, 80);
        printf("%c\n",val);


        mem_sm.print_memory();
        mem_sm.print_swap();
        mem_sm.print_page_table();


    }

