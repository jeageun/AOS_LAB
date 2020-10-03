/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags --libs` fusexmp.c -o fusexmp
*/

#define FUSE_USE_VERSION 26

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
#include <sys/types.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#define PORT 18089
#define MAXLINE 1024

struct net_state {
	struct sockaddr_in* servaddr;
};
#define C_DATA ((struct net_state *) fuse_get_context()->private_data)

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


static int xmp_getattr(int sockfd, struct sockaddr_in *cliaddr)
{
	char buffer[MAXLINE];
	struct stat stbuf; 
	int res;
	int n;
	int len = sizeof(struct sockaddr_in);
	n = recvfrom(sockfd,buffer,MAXLINE,MSG_WAITALL,cliaddr,&len);
	buffer[n]='\0';

	res = lstat(buffer, &stbuf);
	sendto(sockfd,&res,sizeof(int),MSG_CONFIRM,cliaddr,len);
	sendto(sockfd,&stbuf,sizeof(stbuf),MSG_CONFIRM,cliaddr,len);
	return 0;
}

//static int xmp_access(const char *path, int mask)
static int xmp_access(int sockfd, struct sockaddr_in *cliaddr)
{
	int res;
	char buffer[MAXLINE];
	int mask;
	int n;
	int len = sizeof(struct sockaddr_in);
	
	//get path
	n = recvfrom(sockfd,buffer,MAXLINE,MSG_WAITALL,cliaddr,&len);
	buffer[n]='\0';
	
	recvfrom(sockfd,&mask,sizeof(mask),MSG_WAITALL,cliaddr,&len);
	
	res = access(buffer, mask);
	sendto(sockfd,&res,sizeof(int),MSG_CONFIRM,cliaddr,len);

	return 0;
}

//static int xmp_readlink(const char *path, char *buf, size_t size)
static int xmp_readlink(int sockfd, struct sockaddr_in *cliaddr)
{
	int res;
	char buffer[MAXLINE];
	char *buf;
	size_t sz;
	int n;
	int len = sizeof(struct sockaddr_in);
	
	//get path
	n = recvfrom(sockfd,buffer,MAXLINE,MSG_WAITALL,cliaddr,&len);
	buffer[n]='\0';
	
	
	recvfrom(sockfd,&sz,sizeof(sz),MSG_WAITALL,cliaddr,&len);
	
	buf = (char *)malloc(sizeof(char)*sz);
	recvfrom(sockfd,buf,sz,MSG_WAITALL,cliaddr,&len);
	
	res = readlink(buffer, buf, sz - 1);


	sendto(sockfd,&res,sizeof(int),MSG_CONFIRM,cliaddr,len);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	sendto(sockfd,buf,sz,MSG_CONFIRM,cliaddr,len);

	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

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
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(path, mode);
	else
		res = mknod(path, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	int res;

	res = mkdir(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
	int res;

	res = unlink(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	int res;

	res = rmdir(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *to, const char *from)
{
	int res;

	res = symlink(to, from);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	int res;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	int res;

	res = chmod(path, mode);
	if (res == -1)
		return -errno;

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
	int res;

	res = open(path, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void) fi;
	fd = open(path, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void) fi;
	fd = open(path, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
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

int main(int argc, char *argv[])
{
	int sockfd;
	struct sockaddr_in servaddr, cliaddr;


	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));

	// Filling server information
	servaddr.sin_family = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(PORT);
	
	while(1){
		// Creating socket file descriptor
		if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
				perror("socket creation failed");
				exit(EXIT_FAILURE);
		}

		// Bind the socket with the server address
		if ( bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 )
		{
				perror("bind failed");
				exit(EXIT_FAILURE);
		}

		int len, n;
		int opcode;

		len = sizeof(cliaddr); //len is value/resuslt

		n = recvfrom(sockfd, &opcode, sizeof(int), MSG_WAITALL, &cliaddr, &len);
		
	
		printf("Opcode : %d\n", opcode);
		printf("ip : %s\n",inet_ntoa(cliaddr.sin_addr));

		switch (opcode){		
			case GETATTR:
			xmp_getattr(sockfd, &cliaddr);
			break;
			case ACCESS:
			xmp_access(sockfd,&cliaddr);
			break;
			case READLINK:
			xmp_readlink(sockfd,&cliaddr);
			break;

		}
		close(sockfd);
	}
	return 0;

}