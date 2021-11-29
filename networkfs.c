#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>

#include "utils.c"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivanov Ivan");
MODULE_VERSION("0.01");


char fs_token[300];
char name[256 * 3 + 30];
char output_buf[8192 + 10];
char original[512 + 10];
char encode[512 * 3 + 10];


//======================================================


struct inode_operations networkfs_inode_ops;

struct inode *networkfs_get_inode(struct super_block *, const struct inode *, umode_t, int);


//======================================================


struct dentry*
networkfs_lookup(struct inode *parent_inode, struct dentry *child_dentry, unsigned int flag)
{
    char parent[sizeof(ino_t) + 30];
    sprintf(parent, "parent=%lu", parent_inode->i_ino);

    url_encode((const char*)child_dentry->d_name.name, child_dentry->d_name.len, encode);
    sprintf(name, "name=%s", encode);

    const char* params[2];
    params[0] = parent;
    params[1] = name;

    struct entry_info {
        unsigned char entry_type; // DT_DIR (4) or DT_REG (8)
        ino_t ino;
    };

    int status = connect_to_server("lookup", 2, params, fs_token, output_buf);
    
    if (status != 0) {
        char message[200];
        sprintf(message, "lookup error: %d\n", status);
        printk(message);
    } else {
        struct entry_info *info = (struct entry_info*)output_buf;
        struct inode *inode;
        if (info->entry_type == DT_DIR) {
            inode = networkfs_get_inode(parent_inode->i_sb, NULL, S_IFDIR | S_IRWXUGO, (int)info->ino);
        } else {
            inode = networkfs_get_inode(parent_inode->i_sb, NULL, S_IFREG | S_IRWXUGO, (int)info->ino);
        }
        d_add(child_dentry, inode);
    }
    return NULL;
}


//======================================================


int networkfs_create(struct inode *parent_inode, struct dentry *child_dentry, umode_t mode, bool b)
{
    struct inode *inode;

    char parent[sizeof(ino_t) + 30];
    sprintf(parent, "parent=%lu", parent_inode->i_ino);

    url_encode((const char*)child_dentry->d_name.name, child_dentry->d_name.len, encode);
    sprintf(name, "name=%s", encode);

    char *type = "type=file";

    const char* params[3];
    params[0] = parent;
    params[1] = name;
    params[2] = type;

    int status = connect_to_server("create", 3, params, fs_token, output_buf);

    if (status != 0) {
        char message[200];
        sprintf(message, "create error: %d\n", status);
        printk(message);
    } else {
        inode = networkfs_get_inode(parent_inode->i_sb, NULL, S_IFREG | S_IRWXUGO, (int)*(ino_t *)output_buf);
        d_add(child_dentry, inode);
    }

    return status;
}

//====================================================== abacaba


int networkfs_unlink(struct inode *parent_inode, struct dentry *child_dentry)
{
    char parent[sizeof(ino_t) + 30];
    sprintf(parent, "parent=%lu", parent_inode->i_ino);

    url_encode((const char*)child_dentry->d_name.name, child_dentry->d_name.len, encode);
    sprintf(name, "name=%s", encode);

    const char* params[2];
    params[0] = parent;
    params[1] = name;

    int status = connect_to_server("unlink", 2, params, fs_token, output_buf);

    if (status != 0) {
        char message[200];
        sprintf(message, "unlink error: %d\n", status);
        printk(message);
    }

    return status;
}

//======================================================


int networkfs_mkdir(struct inode *parent_inode, struct dentry *child_dentry, umode_t mode) {
    struct inode *inode;

    char parent[sizeof(ino_t) + 30];
    sprintf(parent, "parent=%lu", parent_inode->i_ino);

    url_encode((const char*)child_dentry->d_name.name, child_dentry->d_name.len, encode);
    sprintf(name, "name=%s", encode);

    char *type = "type=directory";

    const char* params[3];
    params[0] = parent;
    params[1] = name;
    params[2] = type;

    int status = connect_to_server("create", 3, params, fs_token, output_buf);

    if (status != 0) {
        char message[200];
        sprintf(message, "mkdir error: %d\n", status);
        printk(message);
    } else {
        inode = networkfs_get_inode(parent_inode->i_sb, NULL, S_IFDIR | S_IRWXUGO, (int)*(ino_t *)output_buf);
        d_add(child_dentry, inode);
    }

    return status;
}


//======================================================


int networkfs_rmdir(struct inode *parent_inode, struct dentry *child_dentry)
{
    char parent[sizeof(ino_t) + 30];
    sprintf(parent, "parent=%lu", parent_inode->i_ino);

    url_encode((const char*)child_dentry->d_name.name, child_dentry->d_name.len, encode);
    sprintf(name, "name=%s", encode);

    const char* params[2];
    params[0] = parent;
    params[1] = name;

    int status = connect_to_server("rmdir", 2, params, fs_token, output_buf);

    if (status != 0) {
        char message[200];
        sprintf(message, "rmdir error: %d\n", status);
        printk(message);
    }

    return status;
}

//======================================================


int networkfs_link(struct dentry *old_dentry, struct inode *parent_dir, struct dentry *new_dentry) {
    char source[sizeof(ino_t) + 30];
    sprintf(source, "source=%lu", old_dentry->d_inode->i_ino);

    char parent[sizeof(ino_t) + 30];
    sprintf(parent, "parent=%lu", parent_dir->i_ino);

    url_encode((const char*)new_dentry->d_name.name, new_dentry->d_name.len, encode);
    sprintf(name, "name=%s", encode);

    const char* params[3];
    params[0] = source;
    params[1] = parent;
    params[2] = name;

    int status = connect_to_server("link", 3, params, fs_token, output_buf);

    d_add(new_dentry, old_dentry->d_inode);
    return status;
}


//======================================================


struct inode_operations networkfs_inode_ops =
{
	.lookup = networkfs_lookup,
    .create = networkfs_create,
    .unlink = networkfs_unlink,
    .mkdir  = networkfs_mkdir,
    .rmdir  = networkfs_rmdir,
    .link   = networkfs_link,
};


//======================================================


int networkfs_iterate(struct file *filp, struct dir_context *ctx)
{
	char fname[256 + 10];
	struct dentry *dentry;
	struct inode *inode;
	unsigned long offset;
	int stored;
	unsigned char ftype;
	ino_t ino;
	ino_t dino;

	dentry = filp->f_path.dentry;
	inode = dentry->d_inode;
	offset = filp->f_pos;
	stored = 0;
	ino = inode->i_ino;

    char inode_param[sizeof(ino_t) + 30];
    sprintf(inode_param, "inode=%lu", ino);

    const char* params[1];
    params[0] = inode_param;

    struct entries {
        size_t entries_count;
        struct entry {
            unsigned char entry_type; // DT_DIR (4) or DT_REG (8)
            ino_t ino;
            char name[256];
        } entries[16];
    };

    int status = connect_to_server("list", 1, params, fs_token, output_buf);

    if (status != 0) {
        char message[200];
        sprintf(message, "iterate error: %d\n", status);
        printk(message);
    } else {
        struct entries *entries = (struct entries*)output_buf;
        while (offset < entries->entries_count) {
            strcpy(fname, entries->entries[offset].name);
            ftype = entries->entries[offset].entry_type;
            dino = entries->entries[offset].ino;

            dir_emit(ctx, fname, (int)strlen(fname), dino, ftype);
            stored++;
            offset++;
            ctx->pos = (loff_t)offset;
        }
    }

	return stored;
}


//======================================================


ssize_t networkfs_read(struct file *filp, char __user *buffer, size_t len, loff_t *offset) {
    struct dentry *dentry;
    struct inode *inode;
    loff_t pos;
    ino_t ino;

    dentry = filp->f_path.dentry;
    inode = dentry->d_inode;
    pos = filp->f_pos;
    ino = inode->i_ino;

    char inode_param[sizeof(ino_t) + 30];
    sprintf(inode_param, "inode=%lu", ino);

    const char* params[1];
    params[0] = inode_param;

    struct content {
        u64 content_length;
        char *content;
    };

    int status = connect_to_server("read", 1, params, fs_token, output_buf);

    if (status != 0) {
        char message[200];
        sprintf(message, "read error: %d\n", status);
        printk(message);
        return -1;
    } else {
        struct content *content = (struct content*)output_buf;
        int i;
        for (i = 0; i < len; i++) { // ???!!!
            //put_user(content->content[pos++], buffer + (*offset)++);
            //*(buffer + (*offset)++) = content->content[pos++];
        }
        return 0;
    }
}


//======================================================


ssize_t networkfs_write(struct file *filp, const char *buffer, size_t len, loff_t *offset) {
    struct dentry *dentry;
    struct inode *inode;
    ino_t ino;

    dentry = filp->f_path.dentry;
    inode = dentry->d_inode;
    ino = inode->i_ino;

    char inode_param[sizeof(ino_t) + 30];
    sprintf(inode_param, "inode=%lu", ino);

    char content[strlen("content=") + 512 * 3 + 10];
    int i = 0;
    for (; i < len; i++) {
        get_user(original[i], buffer + *offset + i); // originaloriginaloriginal
    }
    url_encode(original, len, encode); // ???
    sprintf(content, "content=%s", encode);

    const char* params[2];
    params[0] = inode_param;
    params[1] = content;
    
    int status = connect_to_server("write", 2, params, fs_token, output_buf);

    if (status != 0) {
        char message[200];
        sprintf(message, "write error: %d\n", status);
        printk(message);
        return -1;
    } else {
        return 0;
    }
}


//======================================================


struct file_operations networkfs_dir_ops =
{
	.iterate = networkfs_iterate,
    .read    = networkfs_read,
    .write   = networkfs_write,
};


//======================================================


struct inode *networkfs_get_inode(struct super_block *sb, const struct inode *dir, umode_t mode, int i_ino)
{
	struct inode *inode;
	inode = new_inode(sb);
	if (inode != NULL)
	{
		inode->i_ino = i_ino;
		inode->i_op = &networkfs_inode_ops;
        inode->i_fop = &networkfs_dir_ops;

		inode_init_owner(inode, dir, mode);
	}
	return inode;
}


int networkfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *inode;
	inode = networkfs_get_inode(sb, NULL, S_IFDIR | S_IRWXUGO, 1000);
	sb->s_root = d_make_root(inode);
	if (sb->s_root == NULL)
	{
		return -ENOMEM;
	}
	return 0;
}


struct dentry* networkfs_mount(struct file_system_type *fs_type, int flags, const char *token, void *data)
{
	struct dentry *ret;
	ret = mount_nodev(fs_type, flags, data, networkfs_fill_super);
	if (ret == NULL)
	{
		printk(KERN_ERR "Can't mount file system");
	}
	else
	{
		printk(KERN_INFO "Mounted successfully");
	}
    sprintf(fs_token, "%s", token);
	return ret;
}


void networkfs_kill_sb(struct super_block *sb)
{
	printk(KERN_INFO "networkfs super block is destroyed. Unmount successfully.\n");
}


struct file_system_type networkfs_fs_type =
{
    .name = "networkfs",
    .mount = networkfs_mount,
    .kill_sb = networkfs_kill_sb
};


//======================================================


int networkfs_init(void)
{
	register_filesystem(&networkfs_fs_type);
	return 0;
}

void networkfs_exit(void)
{
	unregister_filesystem(&networkfs_fs_type);
}


module_init(networkfs_init);
module_exit(networkfs_exit);
