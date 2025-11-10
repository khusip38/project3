/* main1.c
 * Project 3: Virtual Memory Simulator – No Page Replacement
 * Numa Wani & Khusi Patel
 * CSC 354-02
 *
 * Reads logical addresses from addresses.txt and prints:
 * 1. Logical address (out1.txt)
 * 2. Corresponding physical address (out2.txt)
 * 3. Signed byte value at that physical address (out3.txt)
 *
 * Reports statistics:
 * Page-fault rate and TLB hit rate
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 256
#define NUM_PAGES 256
#define NUM_FRAMES 256
#define TLB_SIZE 16
#define PHYSICAL_MEMORY_SIZE (NUM_FRAMES * PAGE_SIZE)

typedef struct {
    int page;
    int frame;
} TLBEntry;

unsigned char physical_memory[PHYSICAL_MEMORY_SIZE];
int page_table[NUM_PAGES];
TLBEntry tlb[TLB_SIZE];

int tlb_index = 0;
int next_free_frame = 0;
int tlb_hits = 0;
int page_faults = 0;

int search_tlb(int page) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].page == page) return tlb[i].frame;
    }
    return -1;
}

void add_to_tlb(int page, int frame) {
    tlb[tlb_index % TLB_SIZE].page = page;
    tlb[tlb_index % TLB_SIZE].frame = frame;
    tlb_index++;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s addresses.txt\n", argv[0]);
        return 1;
    }

    FILE *address_file = fopen(argv[1], "r");
    FILE *backing = fopen("BACKING_STORE.bin", "rb");
    FILE *out1 = fopen("out1.txt", "w");
    FILE *out2 = fopen("out2.txt", "w");
    FILE *out3 = fopen("out3.txt", "w");
    if (!address_file || !backing || !out1 || !out2 || !out3) {
        perror("File open error");
        return 1;
    }

    for (int i = 0; i < NUM_PAGES; i++) page_table[i] = -1;
    for (int i = 0; i < TLB_SIZE; i++) tlb[i].page = -1;

    char line[16];
    int total_addresses = 0;

    while (fgets(line, sizeof(line), address_file)) {
        int logical = atoi(line);
        int logical_16 = logical & 0xFFFF;
        int page = (logical_16 >> 8) & 0xFF;
        int offset = logical_16 & 0xFF;

        int frame = search_tlb(page);
        if (frame != -1) {
            tlb_hits++;
        } else if (page_table[page] != -1) {
            frame = page_table[page];
            add_to_tlb(page, frame);
        } else {
            page_faults++;
            fseek(backing, page * PAGE_SIZE, SEEK_SET);
            fread(physical_memory + next_free_frame * PAGE_SIZE, 1, PAGE_SIZE, backing);
            frame = next_free_frame;
            page_table[page] = frame;
            add_to_tlb(page, frame);
            next_free_frame++;
        }

        int physical = frame * PAGE_SIZE + offset;
        signed char value = physical_memory[physical];

        fprintf(out1, "%d\n", logical);
        fprintf(out2, "%d\n", physical);
        fprintf(out3, "%d\n", value);
        total_addresses++;
    }

    printf("Page Fault Rate = %.3f\n", (float)page_faults / total_addresses);
    printf("TLB Hit Rate = %.3f\n", (float)tlb_hits / total_addresses);

    fclose(address_file);
    fclose(backing);
    fclose(out1);
    fclose(out2);
    fclose(out3);
    return 0;
}
