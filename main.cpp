#include <iostream>
#include "sim_mem.h"
#include <queue>


    int main()
    {

        sim_mem mem_sm("exec_file1","","swap_file",25,25,25,25,20,5,1);
        cout<<mem_sm.load(1,37)<<endl;
        cout<<mem_sm.load(1,22)<<endl;
        cout<<mem_sm.load(1,1)<<endl;
        cout<<mem_sm.load(1,37)<<endl;
        cout<<mem_sm.load(1,55)<<endl;
        cout<<mem_sm.load(1,27)<<endl;
        cout<<mem_sm.load(1,31)<<endl;
        cout<<mem_sm.load(1,1)<<endl;
        mem_sm.store(1,22,'x');
        mem_sm.store(1,50,'y');
        mem_sm.store(1,53,'y');
        mem_sm.store(1,80,'w');
        mem_sm.store(1,90,'z');
        mem_sm.print_memory();
        mem_sm.print_swap();
        mem_sm.print_page_table();

        return 0;

        mem_sm.print_memory();
        mem_sm.print_swap();
        mem_sm.print_page_table();


    }

