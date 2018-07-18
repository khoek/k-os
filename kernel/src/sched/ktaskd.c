#include "common/compiler.h"
#include "common/list.h"
#include "bug/debug.h"
#include "mm/mm.h"
#include "sync/spinlock.h"
#include "sync/semaphore.h"

static DEFINE_SPINLOCK(ktaskd_lock);
static DEFINE_LIST(ktaskd_reqlist);
static DEFINE_SEMAPHORE(ktaskd_semaphore, 0);

typedef struct ktaskd_req {
    //TODO allow reqs to set argv and envp

    char *name;
    void (*main)(void *arg);
    void *arg;

    list_head_t list;
} ktaskd_req_t;

void ktaskd_request(char *name, void (*main)(void *arg), void *arg) {
    uint32_t flags;
    spin_lock_irqsave(&ktaskd_lock, &flags);

    ktaskd_req_t *req = kmalloc(sizeof(ktaskd_req_t));
    req->name = name;
    req->main = main;
    req->arg = arg;
    list_add(&req->list, &ktaskd_reqlist);

    semaphore_up(&ktaskd_semaphore);

    spin_unlock_irqstore(&ktaskd_lock, flags);
}

static void ktaskd_fulfil_req(ktaskd_req_t *req) {
    spawn_kernel_task(req->name, req->main, req->arg);
}

static void ktaskd_spawn_pending() {
    semaphore_down(&ktaskd_semaphore);

    ktaskd_req_t *req = NULL;

    uint32_t flags;
    spin_lock_irqsave(&ktaskd_lock, &flags);

    if(!list_empty(&ktaskd_reqlist)) {
        req = list_first(&ktaskd_reqlist, ktaskd_req_t, list);
        list_rm(&req->list);
    }

    spin_unlock_irqstore(&ktaskd_lock, flags);

    if(req) {
        ktaskd_fulfil_req(req);
        kfree(req);
    }
}

static void ktaskd_run(void *UNUSED(arg)) {
    irqenable();

    //Do setpgrp()
    pgroup_rm(current->node);
    pgroup_create(current->node);

    while(true) {
        ktaskd_spawn_pending();
    }
}

void ktaskd_init() {
    spawn_kernel_task("ktaskd", ktaskd_run, NULL);
}
