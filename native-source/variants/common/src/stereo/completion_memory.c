/* Small no-CRT support for isolated upstream MinHook objects. */
typedef unsigned long long mem_size;
void *memcpy(void *target,const void *source,mem_size size) {
    volatile unsigned char *d=target;const volatile unsigned char *s=source;
    for(mem_size i=0;i<size;++i) d[i]=s[i];return target;
}
void *memset(void *target,int value,mem_size size) {
    volatile unsigned char *d=target;
    for(mem_size i=0;i<size;++i) d[i]=(unsigned char)value;return target;
}
