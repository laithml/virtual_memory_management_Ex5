//
// Created by Laith on 26/05/2022.
//
#include <cstdio>
#include <unistd.h>
#include <cstdlib>
#include <sys/fcntl.h>
#include "sim_mem.h"

char main_memory[MEMORY_SIZE];

sim_mem::sim_mem(char exe_file_name1[], char exe_file_name2[], char swap_file_name[], int text_size, int data_size, int bss_size, int heap_stack_size, int num_of_pages, int page_size, int num_of_process) {
    this->text_size = text_size;
    this->data_size = data_size;
    this->bss_size = bss_size;
    this->heap_stack_size = heap_stack_size;
    this->num_of_pages = num_of_pages;
    this->page_size = page_size;
    this->num_of_proc = num_of_process;
    this->textSection = text_size / page_size;
    this->swap_size = (page_size * (num_of_pages - ((text_size + data_size) / page_size)) * num_of_process);
    int swapPage = swap_size / page_size;

    if ((this->swapfile_fd = open(swap_file_name, O_CREAT | O_RDWR | O_TRUNC, 0666)) == -1) {
        perror("can't open the file");
        this->sim_mem::~sim_mem();
        exit(EXIT_FAILURE);
    }
    page_table = new page_descriptor *[num_of_process];
    if (num_of_process == 2) {
        if ((this->program_fd[0] = open(exe_file_name1, O_RDONLY)) == -1 || (this->program_fd[1] = open(exe_file_name2, O_RDONLY)) == -1) {
            perror("can't open the file");
            this->sim_mem::~sim_mem();
            exit(EXIT_FAILURE);
        }
        page_table[0] = new page_descriptor[num_of_pages];
        page_table[1] = new page_descriptor[num_of_pages];
    }
    if (num_of_process == 1) {
        if ((this->program_fd[0] = open(exe_file_name1, O_RDONLY)) == -1) {
            perror("can't open the file");
            this->sim_mem::~sim_mem();
            exit(EXIT_FAILURE);
        }
        page_table[0] = new page_descriptor[num_of_pages];
    }
    int i = 0;
    while (i < MEMORY_SIZE) {
        main_memory[i] = '0';
        i++;
    }
    i = 0;
    while (i < swap_size) {
        if (write(this->swapfile_fd, "0", 1) == -1) {
            perror("Can't write to swap file");
            this->sim_mem::~sim_mem();
            exit(EXIT_FAILURE);
        }
        i++;
    }
    i = 0;
    swap_memory = new int[swapPage];
    while (i < swapPage) {
        swap_memory[i] = -1;
        i++;
    }

    for (int j = 0; j < num_of_process; j++) {
        for (int k = 0; k < num_of_pages; k++) {
            page_table[j][k].V = 0;
            page_table[j][k].D = 0;
            page_table[j][k].frame = -1;
            page_table[j][k].swap_index = -1;
            if (k < textSection)
                page_table[j][k].P = 0;
            else
                page_table[j][k].P = 1;
        }
    }

}

/**************************************************************************************/
sim_mem::~sim_mem() {
    delete[] page_table[0];
    if (num_of_proc == 2)
        delete[] page_table[1];
    delete[] page_table;
    delete[] swap_memory;

    if ((close(swapfile_fd)) == -1 || (close(program_fd[0])) == -1) {
        perror("close Failed");
        exit(EXIT_FAILURE);
    }
    if (num_of_proc == 2) {
        if ((close(program_fd[1])) == -1) {
            perror("close Failed");
            exit(EXIT_FAILURE);
        }
    }
}


char sim_mem::load(int process_id, int address) {
    if (address < 0)
        return '\0';
    process_id--;
    int offset = address % page_size;
    int page = address / page_size;
    int frame;
    if (page_table[process_id][page].V == 1) {
        frame = page_table[process_id][page].frame;
        return main_memory[offset + (frame * page_size)];
    } else {
        if (page_table[process_id][page].P == 0 && page_table[process_id][page].D == 0) {
            if (page <= textSection) {
                char temp[page_size];
                lseek(program_fd[process_id], page_size * page, SEEK_SET);
                if (read(program_fd[process_id], temp, page_size) != -1) {
                    perror("Read From logical memory File Is Failed\n");
                    return '\0';
                }

                int empty = emptyLoc();
                int j = 0;
                for (int i = empty; i < page_size; i++, j++)
                    main_memory[i] = temp[j];

                page_table[process_id][page].V = 1;
                page_table[process_id][page].swap_index = -1;
                page_table[process_id][page].frame = (empty - page_size)/page_size;
                frame = page_table[process_id][page].frame;
                return main_memory[(page_size * frame) + offset];
            } else {//we are in section can't read from it
                fprintf(stderr, "this page doesn't exist, it should be initiate by store function first\n");
                return '\0';
            }
        } else {//V=0,P=1
            if (page_table[process_id][page].D == 1) {
                int index = 0;
                while (index < (swap_size / page_size)) {
                    if (swap_memory[index] == page)
                        break;
                    index++;
                }
                swap_memory[index]=-1;
                char temp[page_size];
                lseek(swapfile_fd, page_size * index, SEEK_SET);
                if (read(swapfile_fd, temp, page_size) != -1) {
                    perror("Read From logical memory File Is Failed\n");
                    return '\0';
                }
                int empty = emptyLoc();
                int j = 0;
                for (int i = empty; i < page_size; i++, j++)
                    main_memory[i] = temp[j];

                page_table[process_id][page].V = 1;
                page_table[process_id][page].swap_index = -1;
                page_table[process_id][page].frame = (empty - page_size)/page_size;
                frame = page_table[process_id][page].frame;
                return main_memory[(page_size * frame) + offset];
            } else {
                fprintf(stderr, "_______________IDK________________\n");
                return '\0';
            }
        }
    }
}

void sim_mem::store(int process_id, int address, char value) {
    if (address < 0)
        return;
    process_id--;
    int offset = address % page_size;
    int page = address / page_size;
    if (page_table[process_id][page].V == 1) {//page is in memory
        main_memory[(page_table[process_id][page].frame * page_size) +offset] = value;
    }else{
        if(page_table[process_id][page].P==0 ) {//v=0,p=0
            fprintf(stderr, "there's no permission to write\n");
            return;
        }
        else if(page<= textSection+(data_size/page_size) ){//v=0,p=1,but cant store here
            fprintf(stderr, "can't store into text/data area\n");
            return;
        }else {
            if (page_table[process_id][page].D == 0) {//v=0,p=1,d=0,not text area
                /*
                 * create a new page at the empty file in the memory
                 * and fill it with '0'
                 * then store the value in the right index
                 */
                int empty = emptyLoc();
                while (empty < page_size) {
                    if (empty == offset)
                        main_memory[empty] = value;
                    else
                        main_memory[empty] = '0';
                    empty++;
                }
                page_table[process_id][page].V = 1;
                page_table[process_id][page].D = 1;
                page_table[process_id][page].frame = (empty - page_size)/page_size;
                page_table[process_id][page].swap_index = -1;
                return;
            }else{
                char temp[page_size];
                int i=0,empty=emptyLoc();
                while(i<swap_size/page_size){
                    if(swap_memory[i]==page)
                        break;
                    i++;
                }
                lseek(swapfile_fd, page_size * i, SEEK_SET);
                if (read(swapfile_fd, temp, page_size) != -1) {
                    perror("Read From Swap File Is Failed\n");
                    return ;
                }
                i=0;
                while(empty<page_size){
                    if(i==offset)
                        main_memory[empty]=value;
                    else
                        main_memory[empty]=temp[i];
                    empty++,i++;
                }
                page_table[process_id][page].frame=(empty - page_size)/page_size;
                page_table[process_id][page].V=1;
                page_table[process_id][page].swap_index=-1;
                return;
            }
        }


    }
}


/**************************************************************************************/
void sim_mem::print_memory() {
    int i;
    printf("\n Physical memory\n");
    for (i = 0; i < MEMORY_SIZE; i++) {
        printf("[%c]\n", main_memory[i]);
    }
}

/************************************************************************************/
void sim_mem::print_swap() {
    char *str = (char *) malloc(this->page_size * sizeof(char));
    int i;
    printf("\n Swap memory\n");
    lseek(swapfile_fd, 0, SEEK_SET); // go to the start of the file
    while (read(swapfile_fd, str, this->page_size) == this->page_size) {
        for (i = 0; i < page_size; i++) {
            printf("%d - [%c]\t", i, str[i]);
        }
        printf("\n");
    }
    free(str);
}

/***************************************************************************************/
void sim_mem::print_page_table() {
    int i;
    for (int j = 0; j < num_of_proc; j++) {
        printf("\n page table of process: %d \n", j + 1);
        printf("Valid\t Dirty\t Permission \t Frame\t Swap index\n");
        for (i = 0; i < num_of_pages; i++) {
            printf("[%d] \t [%d] \t [%d] \t\t\t [%d] \t [%d]\n",
                   page_table[j][i].V,
                   page_table[j][i].D,
                   page_table[j][i].P,
                   page_table[j][i].frame,
                   page_table[j][i].swap_index);
        }
    }
}

int sim_mem::emptyLoc() {
    int i = 0;
    int notEmp = 0;
    while (i < MEMORY_SIZE) {
        notEmp = 0;
        if (main_memory[i] == '0') {
            for (int j = 0; j < page_size; j++) {
                if (main_memory[i] != '0') {
                    notEmp = 1;
                    break;
                }
            }
            if (notEmp == 0)
                break;
        }
        i = i + page_size;
    }
    return i;

}







