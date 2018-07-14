// #include <term.h>
// #include <stdbool.h>
// #include <k/sys.h>
// #include <stdio.h>
//
// int tgetent(char *bp, const char *name) {
//     printf("TE %s %s", bp, name);
//     return MAKE_SYSCALL(unimplemented, "tgetent", true);
// }
//
// int tgetflag(char *id) {
//     return MAKE_SYSCALL(unimplemented, "tgetflag", true);
// }
//
// int tgetnum(char *id) {
//     return MAKE_SYSCALL(unimplemented, "tgetnum", true);
// }
//
// char * tgetstr(char *id, char **area) {
//     MAKE_SYSCALL(unimplemented, "tgetstr", true);
//     return NULL;
// }
//
// char * tgoto(const char *cap, int col, int row) {
//     MAKE_SYSCALL(unimplemented, "tgoto", true);
//     return NULL;
// }
//
// int tputs(const char *str, int affcnt, int (*putc)(int)) {
//     return MAKE_SYSCALL(unimplemented, "tcputs", true);
// }
