#ifndef SHIT_H
#define SHIT_H

void *memcpy(void *restrict dest, const void *restrict src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

void fill(struct limine_framebuffer *framebuffer, uint64_t color);

// Halt and catch fire function.
static void hcf(void) {
    for (;;) {
        __asm__ ("hlt");
    }
}


// Also the shitty CAT code
size_t strlen(const char* str);
void itoa(uint64_t n, char s[]);
void itoa_hex(uint64_t n, char s[]);
void sitoa(int64_t n, char s[]);
void sitoa_hex(int64_t n, char s[]);
void dtoa(double d, int precision, char *str);
double bytes_to_mib(uint64_t bytes);
void reverse(char s[]);

#endif