#include "types.h"
#include "user.h"

#define PGSIZE 4096

void pmalloc_test(){
    void* r = (void*)pmalloc();
    // printf(1,"sanity p-r is %d\n",r);
    if (r == 0)
        printf(1, "pmalloc returned 0.. pmalloc test FAILED\n");
    if(((uint)(r))%4096 == 0)
        printf(1,"pmalloc test PASSED\n");
    else printf(1,"pmalloc test FAILED\n");
    //printf(1, "the address is %x\n", ((uint)r-1)%4096);
}

void protect_page_test(){
    char *buf = "try to write";
    void* r = pmalloc();
    // printf(1,"pmalloced\n");
    void* not_r = malloc(1000);
    // protect_page(r);
    pfree(r);
    r = pmalloc();
    if (protect_page(r) < 0) {
       printf(1,"protect_page returned -1.. protect_page test FAILED\n");
    }

    if (write((int)r, buf, strlen(buf)) < 0)
        printf(1,"protect_page write test PASSED\n");
   else printf(1,"protect_page write test FAILED\n");
    if (protect_page(not_r) < 0)
         printf(1,"protect_page protect test PASSED\n");
    else printf(1,"protect_page protect test FAILED\n");
    free(not_r);
    // printf(1,"protect_page freed real free\n");

    pfree(r);
}
void pfree_test(){
    printf(1,"start\n");
    void* r = pmalloc();
    // printf(1,"after pmalloc\n");
    void* not_r = malloc(1000);
    // printf(1,"after malloc\n");
    protect_page(r);
    if (pfree(not_r) < 0)
        printf(1,"pfree free not_r test PASSED\n");
    else printf(1,"pfree free not_r test FAILED\n");
    if (pfree(r) > 0)
        printf(1,"pfree free r test PASSED\n");
    else printf(1,"pfree free r test FAILED\n");
    
}

void scfifo_test() {
    printf(1, "SCFIFO test\n");
    int i;
    char *pages[31];
    char input[10];

    for (i = 0; i < 12; ++i)
        pages[i] = sbrk(PGSIZE);
    printf(1, "\nThe physical memory should be full - use Ctrl + P followned by Enter\n");
    gets(input, 10);

    printf(1, "\ncreating 3 pages... \n");
    pages[13] = sbrk(PGSIZE);
    pages[14] = sbrk(PGSIZE);
    pages[15] = sbrk(PGSIZE);

    printf(1, "\n\n3 pages are taken out - use Ctrl + P followned by Enter\n\n");
    gets(input, 10);

    printf(1, "\n\nWe accessed 2 pages and then accessed 2 pages we moved to Swapfile. should be 2 PGFLT - use Ctrl + P followned by Enter");
    int j;
    for (j = 3; j < 6; j = j + 2){ 
        pages[j][1]='T';
    }

    pages[0][1] = 'T';
    pages[1][1] = 'E';
     printf(1, "\n\nwe wrote th the first 2 pages - use Ctrl + P followned by Enter\n\n");
    gets(input, 10);

    printf(1,"\n\nTesting the fork :\n\n");

    if (fork() == 0) {
        printf(1, "\nthis is the code of the child%d\n",getpid());
        printf(1, "Child pages are identical to parent - use Ctrl + P followned by Enter\n");
        gets(input, 10);

        pages[10][0] = 'F';
        printf(1, "\n\n a page fault should occur here - use Ctrl + P followned by Enter\n\n");
        gets(input, 10);
        char *command = "/ls";
        char *args[4];
        args[0] = "/ls";
        args[1] = 0; 
        args[2] = 0; 
        if (exec(command,args) < 0) {
            printf(1, "exec failed\n");
        }
        exit();
    }
    else {
        //father waits until child finishes.
        wait();
    }
}
 void lifo_test() {
    printf(1, "LIFO test\n");
    char *command = "/ls";
        char *args[4];
        args[0] = "/ls";
        args[1] = 0; 
        args[2] = 0; 
        if (exec(command,args) < 0) {
            printf(1, "exec failed\n");
        }
        exit();
 }

void none_test() {
    char* pages[45];
    int i = 45;
    printf(1, "None test\n");
    for (i = 0; i < 45; i++) {
        pages[i] = sbrk(PGSIZE);
        printf(1, "pages[%d]=0x%x\n", i, pages[i]);
    }
    char *command = "/ls";
    char *args[4];
    args[0] = "/ls";
    args[1] = 0; 
    args[2] = 0; 
    if (exec(command,args) < 0) {
        printf(1, "exec failed\n");
    }
    printf(1, "None PASSED\n");
}
int main(void) {
    #if SCFIFO
    scfifo_test();
    #elif LIFO
    lifo_test();
    #else
    none_test();
    #endif
    // Task 1
    pmalloc_test();
    protect_page_test();
    pfree_test();
    //Task 2
    printf(1, "THE END..\n");
    exit();
}