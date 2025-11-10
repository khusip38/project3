/* main3.c
 * Project 3: Virtual Memory Simulator – LRU Page Replacement
 * Numa Wani & Khusi Patel
 * CSC 354-02
 *
 * Reads logical addresses from addresses.txt and prints:
 * 1. Logical address (out1.txt)
 * 2. Corresponding physical address (out2.txt)
 * 3. Signed byte value at that physical address (out3.txt)
 *
 * Implements LRU page replacement when physical memory is smaller than virtual memory.
 * Reports statistics:
 * Page-fault rate and TLB hit rate
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 256
#define NUM_PAGES 256
#define TLB_SIZE 16
#define PHYSICAL_MEMORY_MAX (1024*PAGE_SIZE)

typedef struct {
    int page;
    int frame;
    int last_used;
} LRUEntry;

unsigned char physical_memory[PHYSICAL_MEMORY_MAX];
LRUEntry tlb[TLB_SIZE];
int page_table[NUM_PAGES];
int frame_last_used[1024]; // maximum frames

int time_counter = 0;
int tlb_hits = 0;
int page_faults = 0;
int next_free_frame = 0;
int NUM_FRAMES = 128;

int search_tlb(int page) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].page == page) {
            tlb[i].last_used = time_counter++;
            return tlb[i].frame;
        }
    }
    return -1;
}

void add_to_tlb(int page, int frame) {
    int lru_index = 0;
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].page == -1) {
            tlb[i].page = page;
            tlb[i].frame = frame;
            tlb[i].last_used = time_counter++;
            return;
        }
        if (tlb[i].last_used < tlb[lru_index].last_used) lru_index = i;
    }
    tlb[lru_index].page = page;
    tlb[lru_index].frame = frame;
    tlb[lru_index].last_used = time_counter++;
}

int replace_lru_page(FILE *backing, int page) {
    int frame_to_use;
    if (next_free_frame < NUM_FRAMES) {
        frame_to_use = next_free_frame++;
    } else {
        int lru_frame = 0;
        for (int i = 1; i < NUM_FRAMES; i++) {
            if (frame_last_used[i] < frame_last_used[lru_frame]) lru_frame = i;
        }
        frame_to_use = lru_frame;

        for (int i = 0; i < NUM_PAGES; i++) {
            if (page_table[i] == frame_to_use) {
                page_table[i] = -1;
                break;
            }
        }
    }

    fseek(backing, page * PAGE_SIZE, SEEK_SET);
    fread(physical_memory + frame_to_use * PAGE_SIZE, 1, PAGE_SIZE, backing);

    page_table[page] = frame_to_use;
    frame_last_used[frame_to_use] = time_counter++;
    page_faults++;
    add_to_tlb(page, frame_to_use);
    return frame_to_use;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s addresses.txt num_frames\n", argv[0]);
        return 1;
    }

    NUM_FRAMES = atoi(argv[2]);
    if (NUM_FRAMES > 1024) NUM_FRAMES = 1024;

    FILE *address_file = fopen(argv[1], "r");
    FILE *backing = fopen("BACKING_STORE.bin", "rb");
    if (!address_file || !backing) {
        perror("File open error");
        return 1;
    }

    for (int i = 0; i < NUM_PAGES; i++) page_table[i] = -1;
    for (int i = 0; i < TLB_SIZE; i++) tlb[i].page = -1;
    for (int i = 0; i < NUM_FRAMES; i++) frame_last_used[i] = 0;

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
            frame = replace_lru_page(backing, page);
        }

        frame_last_used[frame] = time_counter++;
        int physical = frame * PAGE_SIZE + offset;
        (void)physical; // avoid unused variable
        total_addresses++;
    }

    printf("Page Fault Rate = %.3f\n", (float)page_faults / total_addresses);
    printf("TLB Hit Rate = %.3f\n", (float)tlb_hits / total_addresses);

    fclose(address_file);
    fclose(backing);
    return 0;
}