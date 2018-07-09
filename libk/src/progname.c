static const char *progname;

const char * getprogname() {
    return progname;
}

void setprogname(const char *name) {
    if(!progname) {
        progname = name;
    }
}
