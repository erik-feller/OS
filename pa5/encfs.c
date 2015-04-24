/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  Minor modifications and note by Andy Sayler (2012) <www.andysayler.com>
  Further Modified by Erik Feller and Ryan Talley (2015)

  Source: fuse-2.8.7.tar.gz examples directory
  http://sourceforge.net/projects/fuse/files/fuse-2.X/
  modified source http://www.github.com/erik-feller/OS/pa5/

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags` encfs.c -o pa5-encfs `pkg-config fuse --libs`
  
  This program establishes a mirrored and encrypted file system by specifying a password and a directory to mirror to the program will handle the marking, encrypting and decrypting of the data, on the fly. 

*/

#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//include the aes-crypt file
#include "aes-crypt.c"

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 700
#endif
#define PATHMAX 1024
#define TEMP "/home/user/unsecure/unencrypted"
#define FUSEDATA ((struct priv_data *) fuse_get_context()->private_data)

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

//Declare a global variable to hold the path of the temporary file with decrypted contents
char temppath[1024];

//Set up a struct to hold the mirror directory and the password
struct priv_data {
	char *password;
	char *rootdir;
	char *backup;
};

//function to add the mirror path to the actual path so I can be lazy
static void respath(char destination[PATHMAX], const char *path){
	strcpy(destination, FUSEDATA->rootdir);
	strcat(destination, path);
}

static int encfs_getattr(const char *path, struct stat *stbuf)
{
	int res;
	char realpath[PATHMAX];
	respath(realpath, path);
	res = lstat(realpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int encfs_access(const char *path, int mask)
{
	int res;
	char realpath[PATHMAX];
	respath(realpath, path);
	res = access(realpath, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int encfs_readlink(const char *path, char *buf, size_t size)
{
	int res;
	char realpath[PATHMAX];
	respath(realpath, path);
	res = readlink(realpath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int encfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;
	char realpath[PATHMAX];
	respath(realpath, path);
	dp = opendir(realpath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int encfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	char realpath[PATHMAX];
	respath(realpath, path);
	if (S_ISREG(mode)) {
		res = open(realpath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(realpath, mode);
	else
		res = mknod(realpath, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int encfs_mkdir(const char *path, mode_t mode)
{
	int res;
	char realpath[PATHMAX];
	respath(realpath, path);
	res = mkdir(realpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int encfs_unlink(const char *path)
{
	int res;
	char realpath[PATHMAX];
	respath(realpath, path);
	res = unlink(realpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int encfs_rmdir(const char *path)
{
	int res;
	char realpath[PATHMAX];
	respath(realpath, path);
	res = rmdir(realpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int encfs_symlink(const char *from, const char *to)
{
	int res;
	char realfrom[PATHMAX];
	char realto[PATHMAX];
	respath(realfrom, from);
	respath(realto, to);
	res = symlink(realfrom, realto);
	if (res == -1)
		return -errno;

	return 0;
}

static int encfs_rename(const char *from, const char *to)
{
	int res;
	char realfrom[PATHMAX];
	char realto[PATHMAX];
	respath(realfrom, from);
	respath(realto, to);
	res = rename(realfrom, realto);
	if (res == -1)
		return -errno;

	return 0;
}

static int encfs_link(const char *from, const char *to)
{
	int res;
	char realfrom[PATHMAX];
	char realto[PATHMAX];
	respath(realfrom, from);
	respath(realto, to);
	res = link(realfrom, realto);
	if (res == -1)
		return -errno;

	return 0;
}

static int encfs_chmod(const char *path, mode_t mode)
{
	int res;
	char realpath[PATHMAX];
	respath(realpath, path);
	res = chmod(realpath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int encfs_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;
	char realpath[PATHMAX];
	respath(realpath, path);
	res = lchown(realpath, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int encfs_truncate(const char *path, off_t size)
{
	int res;
	char realpath[PATHMAX];
	respath(realpath, path);
	res = truncate(realpath, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int encfs_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;
	char realpath[PATHMAX];
	respath(realpath, path);
	res = utimes(realpath, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int encfs_open(const char *path, struct fuse_file_info *fi)
{
	int res;
	char realpath[PATHMAX];
	respath(realpath, path);
	res = open(realpath, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int encfs_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;
	//Resolve the mirrored path to get us to the actual file
	char realpath[PATHMAX];
	respath(realpath, path);
	FILE *out_fp;
	FILE *in_fp;
	in_fp = fopen(realpath, "r");
	out_fp = fopen(TEMP,"w");	
	do_crypt(in_fp, out_fp, 0, FUSEDATA->password);	
	fclose(in_fp);
	fclose(out_fp);
	(void) fi;
	fd = open(TEMP, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	in_fp = fopen(TEMP, "r");
	out_fp = fopen(realpath,"w");	
	do_crypt(in_fp, out_fp, 1, FUSEDATA->password);	
	fclose(in_fp);
	fclose(out_fp);
	//truncate(TEMP, 0);
	return res;
}

static int encfs_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	FILE *out_fp;
	FILE *in_fp;
	int fd;
	int res;
	char realpath[PATHMAX];
	respath(realpath, path);
	(void) fi;
	in_fp = fopen(realpath, "r");
	out_fp = fopen(TEMP,"w");	
	printf("unencrypted");
	do_crypt(in_fp, out_fp, 0, FUSEDATA->password);	
	fclose(in_fp);
	fclose(out_fp);
	fd = open(TEMP, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	in_fp = fopen(TEMP, "r");
	out_fp = fopen(realpath,"w");	
	do_crypt(in_fp, out_fp, 1, FUSEDATA->password);	
	fclose(in_fp);
	fclose(out_fp);
//	truncate(TEMP, 0);
	return res;
}

static int encfs_statfs(const char *path, struct statvfs *stbuf)
{
	int res;
	char realpath[PATHMAX];
	respath(realpath, path);
	res = statvfs(realpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int encfs_create(const char* path, mode_t mode, struct fuse_file_info* fi) {

    (void) fi;
    char realpath[PATHMAX];
    respath(realpath, path);
    int res;
    res = creat(realpath, mode);
    if(res == -1)
	return -errno;

    close(res);

    return 0;
}


static int encfs_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
	char realpath[PATHMAX];
	respath(realpath, path);
	(void) realpath;
	(void) fi;
	return 0;
}

static int encfs_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
	char realpath[PATHMAX];
	respath(realpath, path);
	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
static int encfs_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{

	char realpath[PATHMAX];
	respath(realpath, path);
	int res = lsetxattr(realpath, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int encfs_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	char realpath[PATHMAX];
	respath(realpath, path);
	int res = lgetxattr(realpath, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int encfs_listxattr(const char *path, char *list, size_t size)
{
	char realpath[PATHMAX];
	respath(realpath, path);
	int res = llistxattr(realpath, list, size);

	if (res == -1)
		return -errno;
	return res;
}

static int encfs_removexattr(const char *path, const char *name)
{
	//concatenate the mirror path with the filepath. 
	char realpath[PATHMAX];
	respath(realpath, path);
	int res = lremovexattr(realpath, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations encfs_oper = {
	.getattr	= encfs_getattr,
	.access		= encfs_access,
	.readlink	= encfs_readlink,
	.readdir	= encfs_readdir,
	.mknod		= encfs_mknod,
	.mkdir		= encfs_mkdir,
	.symlink	= encfs_symlink,
	.unlink		= encfs_unlink,
	.rmdir		= encfs_rmdir,
	.rename		= encfs_rename,
	.link		= encfs_link,
	.chmod		= encfs_chmod,
	.chown		= encfs_chown,
	.truncate	= encfs_truncate,
	.utimens	= encfs_utimens,
	.open		= encfs_open,
	.read		= encfs_read,
	.write		= encfs_write,
	.statfs		= encfs_statfs,
	.create         = encfs_create,
	.release	= encfs_release,
	.fsync		= encfs_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= encfs_setxattr,
	.getxattr	= encfs_getxattr,
	.listxattr	= encfs_listxattr,
	.removexattr	= encfs_removexattr,
#endif
};

int main(int argc, char *argv[])
{
	umask(0);
	if ((argc < 4)){
		printf("Your arguments are incorrect, should input\npa5-encfs <password> <Mirror Directory> <Mnt Point>");
		return 0;
	}
	//Now need to copy the argv on the heap so that the password, root dir and mirror are in other scopes
	struct priv_data *pass_data = malloc(sizeof(struct priv_data));
	pass_data->password = argv[1];
	pass_data->rootdir = realpath(argv[2],NULL);
	printf("%s", pass_data->password);
	//Now change the argv and argc so that it can be taken passed to fuse main
	char *args[] = { argv[0], argv[3], "-d"};
	return fuse_main(3, args, &encfs_oper, pass_data);
}
