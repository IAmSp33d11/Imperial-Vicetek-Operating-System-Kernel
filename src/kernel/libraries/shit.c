// shit.c
// handles a random collection of shit, hence the name.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <shit.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint8_t *restrict pdest = dest;
    const uint8_t *restrict psrc = src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = dest;
    const uint8_t *psrc = src;

    if ((uintptr_t)src > (uintptr_t)dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if ((uintptr_t)src < (uintptr_t)dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = s1;
    const uint8_t *p2 = s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

void fill(struct limine_framebuffer *framebuffer, uint64_t color) {
	volatile uint32_t *fb_ptr = framebuffer->address;
    for (size_t y = 0; y < framebuffer->height; y++) {
        for (size_t x = 0; x < framebuffer->width; x++) {
            fb_ptr[y * (framebuffer->pitch / 4) + x] = color;
        }
    }
}


// Code from CAT Kernel helpful.c which was a shitty OS. Same with all the comments
// Leave this at the bottom you retard

// Functions I grabbed from my old OS's? OS'es? I dunno my old string.h file.
size_t strlen(const char* str) {
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}


void itoa(uint64_t n, char s[]) {
    uint64_t i;

    i = 0;
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    s[i] = '\0';
    reverse(s);
}

void itoa_hex(uint64_t n, char s[]) {
    uint64_t i;
    i = 0;
    do {
        long digit = n % 16;
        if (digit < 10)
            s[i++] = digit + '0';
        else
            s[i++] = digit - 10 + 'A';
    } while ((n /= 16) > 0);
    s[i] = '\0';
    reverse(s);
}

void sitoa(int64_t n, char s[]) {
    int64_t i, sign;

    if ((sign = n) < 0)
        n = -n;
    i = 0;
    do { 
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

void sitoa_hex(int64_t n, char s[]) {
    int64_t i, sign;

    if ((sign = n) < 0)
        n = -n;
    i = 0;
    do { 
        long digit = n % 16;
        if (digit < 10)
            s[i++] = digit + '0';
        else
            s[i++] = digit - 10 + 'A';
    } while ((n /= 10) > 0);
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

// Ironically this function was drain bamaged for a little while
void dtoa(double d, int precision, char *str) {
    int     int_part;
    size_t  i, len;
    int     decimals;

    int_part = (int)d;
    itoa(int_part, str);
    
    i = strlen(str);
    
    d -= (double)int_part;
    str[i] = '.';
    i++;
    
    decimals = 0;
    while (decimals < precision)
    {
        d *= 10;
        int_part = (int)d;
        str[i] = int_part + '0';
        i++;
        decimals++;
        d -= (double)int_part;
    }
    str[i] = '\0';
}

double bytes_to_mib(uint64_t bytes) {
    double kib = bytes / 1024.0;
    double mib = kib / 1024.0;
    return mib;
}

void reverse(char s[]) {
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}