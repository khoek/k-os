#include "common/init.h"
#include "lib/int.h"
#include "common/list.h"
#include "common/listener.h"
#include "sync/spinlock.h"
#include "fs/vfs.h"

static file_t root_file = {
    .flags = FILE_FLAG_STATIC,

    .ops = NULL,
};

static dentry_t root = {
    .filename = "",
    .file = &root_file,

    .parent = NULL,
    .children = LIST_HEAD(root.children),
};

static DEFINE_LIST(providers);
static DEFINE_SPINLOCK(provider_lock);

void register_vfs_provider(vfs_provider_t *provider) {
    uint32_t flags;
    spin_lock_irqsave(&provider_lock, &flags);

    list_add(&provider->list, &providers);

    spin_unlock_irqstore(&provider_lock, flags);
}

void unregister_vfs_provider(vfs_provider_t *provider) {
    uint32_t flags;
    spin_lock_irqsave(&provider_lock, &flags);

    list_rm(&provider->list);

    spin_unlock_irqstore(&provider_lock, flags);
}

static INITCALL vfs_init() {
    return 0;
}

subsys_initcall(vfs_init);
