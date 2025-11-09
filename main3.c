#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 256
#define NUM_PAGES 256
#define NUM_FRAMES 128   // smaller physical memory
#define TLB_SIZE 16
#define PHYSICAL_MEMORY_SIZE (NUM_FRAMES * PAGE_SIZE)

typedef struct {
    int page;
    int frame;
    int last_used;
} LRUEntry;

unsigned char physical_memory[PHYSICAL_MEMORY_SIZE];
LRUEntry tlb[TLB_SIZE];
int page_table[NUM_PAGES];
int frame_last_used[NUM_FRAMES];

int time_counter = 0;
int tlb_hits = 0;
int page_faults = 0;
int next_free_frame = 0;

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
        if (tlb[i].last_used < tlb[lru_index].last_used)
            lru_index = i;
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
        // Find least recently used frame
        int lru_frame = 0;
        for (int i = 1; i < NUM_FRAMES; i++) {
            if (frame_last_used[i] < frame_last_used[lru_frame])
                lru_frame = i;
        }
        frame_to_use = lru_frame;

        // Invalidate page using this frame
        for (int i = 0; i < NUM_PAGES; i++) {
            if (page_table[i] == frame_to_use) {
                page_table[i] = -1;
                break;
            }
        }
    }

    // Load from BACKING_STORE.bin
    fseek(backing, page * PAGE_SIZE, SEEK_SET);
    fread(physical_memory + frame_to_use * PAGE_SIZE, 1, PAGE_SIZE, backing);

    page_table[page] = frame_to_use;
    frame_last_used[frame_to_use] = time_counter++;
    page_faults++;
    add_to_tlb(page, frame_to_use);
    return frame_to_use;
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
