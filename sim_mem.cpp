//
// Created by Laith on 26/05/2022.
//
#include <cstdio>
#include <unistd.h>
#include <cstdlib>
#include <sys/fcntl.h>
#include "sim_mem.h"

char main_memory[MEMORY_SIZE];
int MEMORY_FRAMES_COUNTER = 0;

sim_mem::sim_mem(char exe_file_name1[], char exe_file_name2[], char swap_file_name[], int text_size, int data_size, int bss_size, int heap_stack_size, int num_of_pages, int page_size, int num_of_process) {
   //constructor
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
    //open the two executable files if there's two process && malloc a  memory for the 2 page_table
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
    //fill the main memory && swapfile zeros
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
    //add new swap array to fill the empty index at the swap file
    swap_memory = new int[swapPage];
    while (i < swapPage) {
        swap_memory[i] = -1;
        i++;
    }

    //init page table
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
    //destructor free all allocated memory and close opened files

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
    delete[] page_table[0];
    if (num_of_proc == 2)
        delete[] page_table[1];
    delete[] page_table;
    delete[] swap_memory;
}


char sim_mem::load(int process_id, int address) {
    if(process_id>num_of_proc){
        fprintf(stderr,"this process not found\n");
        return '\0';
    }
    if (address < 0){
        fprintf(stderr,"the address is negative\n");
        return '\0';
    }
    process_id--;
    int offset = address % page_size;
    int page = address / page_size;
    int frame;
    if(page>=num_of_pages){
        fprintf(stderr,"page out of range");
        exit(EXIT_FAILURE);
    }
    if (page_table[process_id][page].V == 1) {
        frame = page_table[process_id][page].frame;
        return main_memory[offset + (frame * page_size)];
    } else {
        if (page_table[process_id][page].P == 0 || page<textSection+(data_size/page_size)) {
                char temp[page_size];
                lseek(program_fd[process_id], page_size * page, SEEK_SET);
                if (read(program_fd[process_id], temp, page_size) != page_size) {
                    perror("Read From logical memory File Is Failed\n");
                    return '\0';
                }

                int empty = emptyLoc();
                int j = 0;
                for (int i = empty; j < page_size; i++, j++)
                    main_memory[i] = temp[j];

                page_table[process_id][page].V = 1;
                page_table[process_id][page].swap_index = -1;
                page_table[process_id][page].frame = empty / page_size;
                frame = page_table[process_id][page].frame;
                q.push(frame);
                MEMORY_FRAMES_COUNTER++;
                return main_memory[(page_size * frame) + offset];

        } else {//V=0,P=1
            if (page_table[process_id][page].D == 1) {
                int index = 0;
                while (index < (swap_size / page_size)) {
                    if (swap_memory[index] == page)
                        break;
                    index++;
                }
                swap_memory[index] = -1;
                char temp[page_size];
                lseek(swapfile_fd, page_size * index, SEEK_SET);
                if (read(swapfile_fd, temp, page_size) != page_size) {
                    perror("Read From logical memory File Is Failed\n");
                    return '\0';
                }
                int empty = emptyLoc();
                int j = 0;
                for (int i = empty; i < page_size; i++, j++)
                    main_memory[i] = temp[j];

                page_table[process_id][page].V = 1;
                page_table[process_id][page].swap_index = -1;
                page_table[process_id][page].frame = empty / page_size;
                frame = page_table[process_id][page].frame;
                q.push(frame);
                MEMORY_FRAMES_COUNTER++;
                return main_memory[(page_size * frame) + offset];
            } else {
                if(page>textSection && page<=((data_size+bss_size)/page_size) +textSection){
                    int empty =emptyLoc();
                    for (int i = empty; i <page_size ; ++i) {
                        main_memory[i]='0';
                    }
                    page_table[process_id][page].V=1;
                    page_table[process_id][page].frame=empty/page_size;
                    return main_memory[empty+offset];
                }else {
                    fprintf(stderr, "this page doesn't exist, it should be initiate by store function first\n");
                    return '\0';
                }

            }
        }
    }
}

void sim_mem::store(int process_id, int address, char value) {
    if(process_id>num_of_proc){
        fprintf(stderr,"this process not found\n");
        return ;
    }
    if (address < 0){
        fprintf(stderr,"the address is negative\n");
        return ;
    }
    process_id--;
    int offset = address % page_size;
    int page = address / page_size;
    if(page>=num_of_pages){
        fprintf(stderr,"page out of range");
        exit(EXIT_FAILURE);
    }


    if (page_table[process_id][page].V == 1) {//page is in memory
        main_memory[(page_table[process_id][page].frame * page_size) + offset] = value;
        page_table[process_id][page].D=1;
        MEMORY_FRAMES_COUNTER++;
    } else {
        if (page_table[process_id][page].P == 0) {//v=0,p=0
            fprintf(stderr, "there's no permission to write\n");
            return;
        }  else {
            if(page < textSection+(bss_size/page_size)){//data section
                char temp[page_size];
                lseek(program_fd[process_id], page_size * page, SEEK_SET);
                if (read(program_fd[process_id], temp, page_size) != page_size) {
                    perror("Read From logical memory File Is Failed\n");
                    return;
                }

                int empty = emptyLoc();
                int j = 0;
                for (int i = empty; j < page_size; i++, j++)
                    main_memory[i] = temp[j];

                empty=empty/page_size;
                main_memory[(page_size * empty) + offset]=value;
                page_table[process_id][page].V=1;
                page_table[process_id][page].D=1;
                page_table[process_id][page].frame=empty;
                q.push(empty);
                return;

            }
            if (page_table[process_id][page].D == 0) {//v=0,p=1,d=0,not text area
                /*
                 * create a new page at the empty file in the memory
                 * and fill it with '0'
                 * then store the value in the right index
                 */
                int empty = emptyLoc(), i = 0;
                while (i < page_size) {
                    if (i == offset)
                        main_memory[empty] = value;
                    else
                        main_memory[empty] = '0';
                    i++;
                    empty++;
                }
                page_table[process_id][page].V = 1;
                page_table[process_id][page].D = 1;
                page_table[process_id][page].frame = (empty - page_size) / page_size;
                q.push(page_table[process_id][page].frame);
                MEMORY_FRAMES_COUNTER++;
                page_table[process_id][page].swap_index = -1;
                return;
            } else {
                char temp[page_size];
                int i = 0, empty = emptyLoc();
                while (i < swap_size / page_size) {
                    if (swap_memory[i] == page)
                        break;
                    i++;
                }
                lseek(swapfile_fd, page_size * i, SEEK_SET);
                if (read(swapfile_fd, temp, page_size) != page_size) {
                    perror("Read From Swap File Is Failed\n");
                    return;
                }
                i = 0;
                while (i < page_size) {
                    if (i == offset)
                        main_memory[empty] = value;
                    else
                        main_memory[empty] = temp[i];
                    empty++, i++;
                }
                page_table[process_id][page].frame = (empty - page_size) / page_size;
                page_table[process_id][page].V = 1;
                q.push(page_table[process_id][page].frame);
                MEMORY_FRAMES_COUNTER++;
                page_table[process_id][page].swap_index = -1;
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
        printf("\n page table of process: %d \n", j);
        printf("Valid\t Dirty\t Permission \t Frame\t Swap index\n");
        for(i = 0; i < num_of_pages; i++) {
            printf("[%d]\t[%d]\t[%d]\t[%d]\t[%d]\n",
                   page_table[j][i].V,
                   page_table[j][i].D,
                   page_table[j][i].P,
                   page_table[j][i].frame ,
                   page_table[j][i].swap_index);
        }
    }
}


/*
 * this function return the empty frame at the memory
 * and if there's no empty place, it calls a freeLoc function
 */
int sim_mem::emptyLoc() {
    int i = 0;
    int notEmp = 0;
    if (MEMORY_FRAMES_COUNTER >= MEMORY_SIZE / page_size) {
        freeLoc();
    }

    while (i < MEMORY_SIZE) {
        notEmp = 0;
        if (main_memory[i] == '0') {
            for (int j = i; j < page_size; j++) {
                if (main_memory[j] != '0') {
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

/*
 * freeLoc check the frame that want to free according to the index from the queue
 * if it's not dirty and it from text section just override it
 * else move it to swap file
 *
 */
void sim_mem::freeLoc() {
    int index = q.front();
    q.pop();
    int i = 0;
    int j = 0;
    int found=0;
    for (; j < num_of_proc; j++) {
        for(i=0;i<num_of_pages;i++) {
            if (page_table[j][i].frame == index){
                found=1;
                break;
            }
        }
        if(found==1){
            break;
        }
    }
    index*=page_size;
    if (page_table[j][i].D == 0) {
        for (int k = 0; k < page_size; k++) {
            main_memory[index] = '0';
            index++;
        }
        page_table[j][i].frame=-1;
        page_table[j][i].V=0;
        page_table[j][i].swap_index=-1;
    } else {
        int swap_index = 0;
        for (; swap_index < swap_size / page_size; swap_index++) {
            if (swap_memory[swap_index] == -1)
                break;
        }
        char temp[page_size];
        for (int l = 0; l < page_size; l++) {
            temp[l] = main_memory[index];
            main_memory[index] = '0';
            index++;
        }
        lseek(swapfile_fd, swap_index * page_size, SEEK_SET);
        if (write(swapfile_fd, temp, page_size) == -1) {
            perror("Can't write to swap file");
            this->sim_mem::~sim_mem();
            exit(EXIT_FAILURE);
        }
        swap_memory[swap_index]=i;
        page_table[j][i].V=0;
        page_table[j][i].frame=-1;
        page_table[j][i].swap_index=swap_index;

    }
}