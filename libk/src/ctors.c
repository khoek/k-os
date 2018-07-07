extern void __do_global_ctors_aux();
extern void __do_global_dtors_aux();

//FIXME why don't we need these?/do we?

void __do_global_ctors() {
    // __do_global_ctors_aux();
}

void __do_global_dtors() {
    // __do_global_dtors_aux();
}
