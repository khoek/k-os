void die() __attribute__((noreturn));

void panic(char* message) __attribute__((noreturn));
void panicf(char* fmt, ...) __attribute__((noreturn));
