/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags --libs` fusexmp.c -o fusexmp
*/

#define FUSE_USE_VERSION 26
#include "arguments.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include <stdlib.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#define PORT 18089
#define MAXLINE 1024

enum operations{
  GETATTR ,
  READLINK ,
  GETDIR ,
  MKNOD ,
  MKDIR ,
  UNLINK ,
  RMDIR ,
  SYMLINK ,
  RENAME ,
  LINK ,
  CHMOD ,
  CHOWN ,
  TRUNCATE ,
  UTIME ,
  OPEN ,
  READ ,
  WRITE ,
  STATFS ,
  FLUSH ,
  RELEASE ,
  FSYNC ,
  OPENDIR ,
  READDIR ,
  RELEASEDIR ,
  FSYNCDIR ,
  INIT ,
  DESTROY ,
  ACCESS ,
  FTRUNCATE ,
  FGETATTR 
};

struct net_state {
	struct sockaddr_in* servaddr;
};
#define C_DATA ((struct net_state *) ((struct fuse_context*)fuse_get_context())->private_data)

_val send_through_net(const char *path,int opcode, struct _args *arg)
{
	int sockfd;
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
	_val res;
	int len = sizeof(*C_DATA->servaddr);
	sendto(sockfd, &opcode, sizeof(int),MSG_CONFIRM,C_DATA->servaddr, len);
	
	switch(opcode){
case GETATTR:
	printf("GETATTR\n");
	fflush(stdout);
	sendto(sockfd, path, strlen(path),MSG_CONFIRM, C_DATA->servaddr, len);
	recvfrom(sockfd,&res,sizeof(_val),MSG_WAITALL, C_DATA->servaddr, &len);
	recvfrom(sockfd,arg->_stbuf,sizeof(*arg->_stbuf),MSG_WAITALL,C_DATA->servaddr,&len);
	break;
case ACCESS:
	printf("ACCESS\n");
	fflush(stdout);
	sendto(sockfd, path, strlen(path),MSG_CONFIRM, C_DATA->servaddr, len);
	sendto(sockfd, arg, sizeof(struct _args),MSG_CONFIRM, C_DATA->servaddr, len);
	recvfrom(sockfd,&res,sizeof(_val),MSG_WAITALL, C_DATA->servaddr, &len);
	break;
case READLINK:
	printf("READLINK\n");
	fflush(stdout);
	sendto(sockfd, path, strlen(path),MSG_CONFIRM, C_DATA->servaddr, len);
	sendto(sockfd, &arg->_size,sizeof(size_t),MSG_CONFIRM, C_DATA->servaddr, len);
	sendto(sockfd, arg->_buf, arg->_size ,MSG_CONFIRM, C_DATA->servaddr, len);
	recvfrom(sockfd,&res,sizeof(_val),MSG_WAITALL, C_DATA->servaddr, &len);
	if(res._res >=0){ 
		recvfrom(sockfd,arg->_buf,arg->_size,MSG_WAITALL, C_DATA->servaddr, &len);
	}
	break;
case READDIR:
	printf("READDIRi\n");
	fflush(stdout);
	sendto(sockfd, path, strlen(path),MSG_CONFIRM, C_DATA->servaddr, len);
	recvfrom(sockfd,&res,sizeof(_val),MSG_WAITALL, C_DATA->servaddr, &len);
	if(res._res >= 0){
		recvfrom(sockfd,&arg->_size,sizeof(size_t),MSG_WAITALL, C_DATA->servaddr, &len);
		arg->_data = malloc(sizeof(struct dirent)*arg->_size);
		recvfrom(sockfd,arg->_data,sizeof(struct dirent)*arg->_size,MSG_WAITALL, C_DATA->servaddr, &len);
	}
	break;
case MKDIR:
	printf("MKDIR\n");
	fflush(stdout);
	sendto(sockfd, path, strlen(path),MSG_CONFIRM, C_DATA->servaddr, len);
	sendto(sockfd, &arg->_mode,sizeof(mode_t),MSG_CONFIRM, C_DATA->servaddr,len);
	recvfrom(sockfd,&res,sizeof(_val),MSG_WAITALL, C_DATA->servaddr, &len);
	break;
case UNLINK:
	printf("UNLINK\n");
	fflush(stdout);
	sendto(sockfd, path, strlen(path),MSG_CONFIRM, C_DATA->servaddr, len);
	recvfrom(sockfd,&res,sizeof(_val),MSG_WAITALL, C_DATA->servaddr, &len);
	break;
case RMDIR:
	printf("RMDIR\n");
	fflush(stdout);
	sendto(sockfd, path, strlen(path),MSG_CONFIRM, C_DATA->servaddr, len);
	recvfrom(sockfd,&res,sizeof(_val),MSG_WAITALL, C_DATA->servaddr, &len);
	break;
case SYMLINK:
	printf("SYMLINK\n");
	fflush(stdout);
	sendto(sockfd, path, strlen(path),MSG_CONFIRM, C_DATA->servaddr, len);
	recvfrom(sockfd,&res,sizeof(_val),MSG_WAITALL, C_DATA->servaddr, &len);
	break;	
case RENAME:
	printf("RENAME\n");
	fflush(stdout);
	sendto(sockfd, path, strlen(path),MSG_CONFIRM, C_DATA->servaddr, len);
	recvfrom(sockfd,&res,sizeof(_val),MSG_WAITALL, C_DATA->servaddr, &len);
	break;	
case LINK:
	printf("LINK\n");
	fflush(stdout);
	sendto(sockfd, path, strlen(path),MSG_CONFIRM, C_DATA->servaddr, len);
	recvfrom(sockfd,&res,sizeof(_val),MSG_WAITALL, C_DATA->servaddr, &len);
	break;
case OPEN:
	printf("OPEN\n");
	fflush(stdout);
	sendto(sockfd, path, strlen(path),MSG_CONFIRM, C_DATA->servaddr, len);
	sendto(sockfd, arg->_fi, sizeof(struct fuse_file_info) ,MSG_CONFIRM, C_DATA->servaddr, len);
	recvfrom(sockfd,&res,sizeof(_val),MSG_WAITALL, C_DATA->servaddr, &len);
	break;
case READ:
	printf("READ\n");
	fflush(stdout);
	sendto(sockfd, path, strlen(path),MSG_CONFIRM, C_DATA->servaddr, len);
	sendto(sockfd, arg, sizeof(struct _args),MSG_CONFIRM, C_DATA->servaddr, len);
	recvfrom(sockfd,&res,sizeof(_val),MSG_WAITALL, C_DATA->servaddr, &len);
	if(res._res >= 0){
		arg->_data = malloc(arg->_size);
		recvfrom(sockfd,arg->_data,arg->_size,MSG_WAITALL, C_DATA->servaddr, &len);
	}
	break;
case WRITE:
	printf("WRITE\n");
	fflush(stdout);

	sendto(sockfd, path, strlen(path),MSG_CONFIRM, C_DATA->servaddr, len);
	sendto(sockfd, arg, sizeof(struct _args),MSG_CONFIRM, C_DATA->servaddr, len);
	sendto(sockfd, arg->_data, arg->_size,MSG_CONFIRM, C_DATA->servaddr, len);
	recvfrom(sockfd,&res,sizeof(_val),MSG_WAITALL, C_DATA->servaddr, &len);
	break;
case CHMOD:
	printf("CHMOD\n");
	fflush(stdout);
	
	sendto(sockfd, path, strlen(path),MSG_CONFIRM, C_DATA->servaddr, len);
	sendto(sockfd, &arg->_mode, sizeof(mode_t),MSG_CONFIRM, C_DATA->servaddr, len);
	recvfrom(sockfd,&res,sizeof(_val),MSG_WAITALL, C_DATA->servaddr, &len);
	break;
case MKNOD:
	printf("MKNOD\n");
	fflush(stdout);

	sendto(sockfd, path, strlen(path),MSG_CONFIRM, C_DATA->servaddr, len);
	sendto(sockfd, &arg->_mode, sizeof(mode_t),MSG_CONFIRM, C_DATA->servaddr, len);
	recvfrom(sockfd,&res,sizeof(_val),MSG_WAITALL, C_DATA->servaddr, &len);
	break;
	}
	close(sockfd);
	return res;

}
//DONE
static int xmp_getattr(const char *path, struct stat *stbuf)
{
	_val res;
	struct _args arg;
	arg._stbuf = stbuf;
	res = send_through_net(path,GETATTR,&arg);

	//res = lstat(path, stbuf);
	if (res._res == -1)
		return res._errno;

	return 0;
}

//DONE
static int xmp_access(const char *path, int mask)
{
	_val res;
	struct _args arg;
	arg._mask = mask;
	res = send_through_net(path,ACCESS,&arg);
	//res = access(path, mask);
	if (res._res == -1)
		return res._errno;

	return 0;
}

//DONE
static int xmp_readlink(const char *path, char *buf, size_t size)
{
	_val res;
	struct _args arg;
	arg._buf = (void*)buf;
	arg._size = size;
	res = send_through_net(path,READLINK,&arg);
	//res = readlink(path, buf, size - 1);
	if (res._res == -1)
		return res._errno;

	buf[res._res] = '\0';
	return 0;
}


//DONE
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;
	struct _args arg;	

	(void) offset;
	(void) fi;
	
	arg._buf = buf;

	_val res = send_through_net(path,READDIR,&arg);
	if (res._res >=0){
		for (int i=0;i<arg._size;i++){
			if(filler(buf,((struct dirent*)arg._data)[i].d_name,NULL,0) != 0){
				free(arg._data);
				return -1;
			}
		}
		free(arg._data);
	}
	/*
	dp = opendir(path);
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
	*/
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	
	_val res;
	struct _args arg;
	arg._mode = mode;
	res = send_through_net(path,MKNOD,&arg);


	if (res._res == -1)
		return res._errno;

	return 0;
}

//DONE
static int xmp_mkdir(const char *path, mode_t mode)
{
	_val res;
	struct _args arg;
	arg._mode = mode;
	res = send_through_net(path,MKDIR,&arg);
	//res = mkdir(path, mode);
	if (res._res == -1)
		return res._errno;

	return 0;
}

//DONE
static int xmp_unlink(const char *path)
{
	_val res;
	struct _args arg;

	res = send_through_net(path,UNLINK,&arg);
	//res = unlink(path);
	if (res._res == -1)
		return res._errno;

	return 0;
}

//DONE
static int xmp_rmdir(const char *path)
{
	_val res;
	struct _args arg;
	res = send_through_net(path,RMDIR,&arg);
	//res = rmdir(path);
	if (res._res == -1)
		return res._errno;

	return 0;
}

//DONE
static int xmp_symlink(const char *to, const char *from)
{
	_val res;
	char catstr[2*MAXLINE];
	strcpy(catstr,from);
	strcat(catstr,"\27");//27 is DEL, not available in usual file system. So, Magic number.
	strcat(catstr,to);
	
	res = send_through_net(catstr,SYMLINK,NULL);

	
	//res = symlink(to, from);
	if (res._res == -1)
		return res._errno;

	return 0;
}

//DONE
static int xmp_rename(const char *from, const char *to)
{
	_val res;
	char catstr[2*MAXLINE];
	strcpy(catstr,from);
	strcat(catstr,"\27");//27 is DEL, not available in usual file system. So, Magic number.
	strcat(catstr,to);
	
	res = send_through_net(catstr,RENAME,NULL);

	//res = rename(from, to);
	if (res._res == -1)
		return res._errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	_val res;
	char catstr[2*MAXLINE];
	strcpy(catstr,from);
	strcat(catstr,"\27");//27 is DEL, not available in usual file system. So, Magic number.
	strcat(catstr,to);
	
	res = send_through_net(catstr,LINK,NULL);

	//res = link(from, to);
	if (res._res == -1)
		return res._errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	_val res;
	struct _args arg;
	arg._mode = mode;
	res = send_through_net(path,CHMOD,&arg);
	//res = chmod(path, mode);
	if (res._res == -1)
		return res._errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;

	res = lchown(path, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	int res;

	res = truncate(path, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(path, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	_val res;
	struct _args arg;
	arg._fi = fi;
	res = send_through_net(path,OPEN,&arg);
	//res = open(path, fi->flags);
	if (res._res == -1)
		return res._errno;

	close(res._res);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	_val res;

	(void) fi;
	struct _args arg;
	arg._size = size;
	arg._offset = offset;
	res = send_through_net(path,READ,&arg);
	if (res._res>=0){
		memcpy(buf,arg._data,arg._size);
		free(arg._data);
	}else{
		return res._errno;
	}
	return res._res;
	//fd = open(path, O_RDONLY);
	//if (fd == -1)
	//	return -errno;
	//
	//res = pread(fd, buf, size, offset);
	//if (res == -1)
	//	res = -errno;

	//close(fd);
	//return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int fd;
	_val res;

	(void) fi;
	
	struct _args arg;
	arg._size = size;
	arg._offset = offset;
	arg._data = buf;
	res = send_through_net(path,WRITE,&arg);
	if(res._res==-1)
		return res._errno;
	return res._res;
	
	/*
	fd = open(path, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
	*/
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;

	res = statvfs(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	int res = lsetxattr(path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	int res = lgetxattr(path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	int res = llistxattr(path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	int res = lremovexattr(path, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

void *cl_init(struct fuse_conn_info *conn)
{
	return C_DATA;
}

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
	.utimens	= xmp_utimens,
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
};

int main(int argc, char *argv[])
{
	struct sockaddr_in _servaddr;
	struct net_state* _state;

	memset(&_servaddr,0,sizeof(_servaddr));
	_servaddr.sin_family = AF_INET;
	_servaddr.sin_port = htons(PORT);
	_servaddr.sin_addr.s_addr = inet_addr("192.168.0.160");

	_state = malloc(sizeof(struct net_state));
	_state->servaddr = &_servaddr;

	umask(0);
	return fuse_main(argc, argv, &xmp_oper, _state);
}
