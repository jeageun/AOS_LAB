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
#include <sys/types.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#define PORT 18089
#define MAXLINE 1024

char ROOTPATH[MAXLINE];
struct net_state {
	struct sockaddr_in* servaddr;
};
#define C_DATA ((struct net_state *) fuse_get_context()->private_data)

enum operations{
  GETATTR ,//0
  READLINK ,//1
  GETDIR ,//2
  MKNOD ,//3
  MKDIR ,//4
  UNLINK ,//5
  RMDIR ,//6
  SYMLINK ,//7
  RENAME ,//8
  LINK ,//9
  CHMOD ,//10
  CHOWN ,//11
  TRUNCATE ,//12
  UTIME ,//13
  OPEN ,//14
  READ ,//15
  WRITE ,//16
  STATFS ,//17
  FLUSH ,//18
  RELEASE ,//19
  FSYNC ,//20
  OPENDIR ,//21
  READDIR ,//22
  RELEASEDIR ,//23
  FSYNCDIR ,//24
  INIT ,//25
  DESTROY ,//26
  ACCESS ,//27
  FTRUNCATE ,//28
  FGETATTR//29
};

static void fullpath(char fpath[MAXLINE], const char *path)
{
	strcpy(fpath,ROOTPATH);
	strncat(fpath,path,ROOTPATH);
}

static int xmp_getattr(int sockfd, struct sockaddr_in *cliaddr)
{
	char buffer[MAXLINE];
	char fpath[MAXLINE];

	struct stat stbuf; 
	_val res;
	int n;
	int len = sizeof(struct sockaddr_in);
	n = recvfrom(sockfd,buffer,MAXLINE,MSG_WAITALL,cliaddr,&len);
	buffer[n]='\0';


	fullpath(fpath,buffer);
	res._res = lstat(fpath, &stbuf);

	if(res._res == -1)
		res._errno=-errno;

	sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);
	sendto(sockfd,&stbuf,sizeof(stbuf),MSG_CONFIRM,cliaddr,len);
	return 0;
}

//static int xmp_access(const char *path, int mask)
static int xmp_access(int sockfd, struct sockaddr_in *cliaddr)
{
	_val res;
	char buffer[MAXLINE];
	char fpath[MAXLINE];
	struct _args arg;
	int n;
	int len = sizeof(struct sockaddr_in);
	
	//get path
	n = recvfrom(sockfd,buffer,MAXLINE,MSG_WAITALL,cliaddr,&len);
	buffer[n]='\0';
	
	fullpath(fpath,buffer);
	recvfrom(sockfd,&arg,sizeof(struct _args),MSG_WAITALL,cliaddr,&len);
	
	res._res = access(fpath, arg._mask);
	if(res._res == -1)
		res._errno = -errno;
	sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);

	return 0;
}

//static int xmp_readlink(const char *path, char *buf, size_t size)
static int xmp_readlink(int sockfd, struct sockaddr_in *cliaddr)
{
	_val res;
	char buffer[MAXLINE];
	char fpath[MAXLINE];
	char *buf;
	size_t sz;
	int n;
	int len = sizeof(struct sockaddr_in);
	
	//get path
	n = recvfrom(sockfd,buffer,MAXLINE,MSG_WAITALL,cliaddr,&len);
	buffer[n]='\0';
	
	fullpath(fpath,buffer);
	
	recvfrom(sockfd,&sz,sizeof(sz),MSG_WAITALL,cliaddr,&len);
	
	buf = (char *)malloc(sizeof(char)*sz);
	recvfrom(sockfd,buf,sz,MSG_WAITALL,cliaddr,&len);
	
	res._res = readlink(fpath, buf, sz - 1);
	
	if (res._res == -1)
		res._errno = -errno;

	sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);
	if (res._res == -1)
		return -errno;

	buf[res._res] = '\0';
	sendto(sockfd,buf,sz,MSG_CONFIRM,cliaddr,len);

	return 0;
}


//static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
//		       off_t offset, struct fuse_file_info *fi)
static int xmp_readdir(int sockfd, struct sockaddr_in *cliaddr)
{
	DIR *dp;
	struct dirent *de;
	struct dirent *de_arr;
	
	char path[MAXLINE];
	char fpath[MAXLINE];

	size_t n=0;
	int len = sizeof(struct sockaddr_in);
	
	//get path
	n = recvfrom(sockfd,path,MAXLINE,MSG_WAITALL,cliaddr,&len);
	path[n]='\0';

	fullpath(fpath,path);

	_val res;
	dp = opendir(fpath);
	if (dp == NULL){
		res._res = -1;
		res._errno = -errno;
		sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);
		return -errno;
	}
	res._res = 0;
	sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);
	de_arr = (struct dirent*)malloc(sizeof(struct dirent)*MAXLINE);
	n=0;	
	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		memcpy((void*)&(de_arr[n++]),de,sizeof(struct dirent));
	}
	
	sendto(sockfd,&n,sizeof(size_t),MSG_CONFIRM,cliaddr,len);
	sendto(sockfd,de_arr,sizeof(struct dirent)*n,MSG_CONFIRM,cliaddr,len);

	closedir(dp);
	return 0;
}

//static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
static int xmp_mknod(int sockfd, struct sockaddr_in *cliaddr)
{

	char path[MAXLINE];
	char fpath[MAXLINE];

	size_t n=0;
	int len = sizeof(struct sockaddr_in);
	mode_t mode;
	_val res;
	
	//get path
	n = recvfrom(sockfd,path,MAXLINE,MSG_WAITALL,cliaddr,&len);
	path[n]='\0';	
	recvfrom(sockfd,&mode,sizeof(mode_t),MSG_WAITALL,cliaddr,&len);
	fullpath(fpath,path);

	res._res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
	if(res._res>=0) res._res =  close(res._res);
	if (res._res == -1)
		res._errno = -errno;

	sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable 
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

	*/
}

//static int xmp_mkdir(const char *path, mode_t mode)
static int xmp_mkdir(int sockfd, struct sockaddr_in *cliaddr)
{
	char path[MAXLINE];
	char fpath[MAXLINE];
	size_t n=0;
	int len = sizeof(struct sockaddr_in);
	mode_t mode;
	_val res;
	
	//get path
	n = recvfrom(sockfd,path,MAXLINE,MSG_WAITALL,cliaddr,&len);
	path[n]='\0';	
	recvfrom(sockfd,&mode,sizeof(mode_t),MSG_WAITALL,cliaddr,&len);
	fullpath(fpath,path);
	res._res = mkdir(fpath, mode);

	if (res._res == -1)
		res._errno =  -errno;

	sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);
	return 0;
}

//static int xmp_unlink(const char *path)
static int xmp_unlink(int sockfd, struct sockaddr_in *cliaddr)
{
	_val res;
	char path[MAXLINE];
	char fpath[MAXLINE];
	int len = sizeof(struct sockaddr_in);
	size_t n=0;
	
	//get path
	n = recvfrom(sockfd,path,MAXLINE,MSG_WAITALL,cliaddr,&len);
	path[n]='\0';	
	fullpath(fpath,path);

	res._res = unlink(fpath);
	if (res._res == -1)
		res._errno=-errno;

	sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);
	return 0;
}

//static int xmp_rmdir(const char *path)
static int xmp_rmdir(int sockfd, struct sockaddr_in *cliaddr)
{
	_val res;
	char path[MAXLINE];
	char fpath[MAXLINE];
	int len = sizeof(struct sockaddr_in);
	size_t n=0;	
	//get path
	n = recvfrom(sockfd,path,MAXLINE,MSG_WAITALL,cliaddr,&len);
	path[n]='\0';	
	fullpath(fpath,path);

	res._res = rmdir(fpath);
	if (res._res == -1)
		 res._errno=-errno;
	sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);

	return 0;
}

//static int xmp_symlink(const char *to, const char *from)
static int xmp_symlink(int sockfd, struct sockaddr_in *cliaddr)
{
	_val res;
	char path[2*MAXLINE];
	char f_from[MAXLINE];
	char f_to[MAXLINE];
	char *from;
	char *to;

	int len = sizeof(struct sockaddr_in);
	size_t n=0;	
	//get path
	n = recvfrom(sockfd,path,2*MAXLINE,MSG_WAITALL,cliaddr,&len);
	path[n]='\0';	

	char * token = strtok(path, "\27");
	// loop through the string to extract all other tokens
	from=(char*)malloc(strlen(token)+1);
	strcpy(from,token);
	token = strtok(NULL, "\27");
	
	to=(char*)malloc(strlen(token)+1);
	strcpy(to,token);

	fullpath(f_from,from);
	fullpath(f_to,to);
	
	printf("%s\n",f_from);
	printf("%s\n",f_to);

	res._res = symlink(f_to, f_from);
	if (res._res == -1)
		res._errno=-errno;
	sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);
	
	free(from);
	free(to);

	return 0;
}

//static int xmp_rename(const char *from, const char *to)
static int xmp_rename(int sockfd, struct sockaddr_in *cliaddr)
{
	_val res;
	char path[2*MAXLINE];
	char *from;
	char *to;
	char f_from[MAXLINE];
	char f_to[MAXLINE];


	int len = sizeof(struct sockaddr_in);
	size_t n=0;	
	//get path
	n = recvfrom(sockfd,path,2*MAXLINE,MSG_WAITALL,cliaddr,&len);
	path[n]='\0';	

	char * token = strtok(path, "\27");
	// loop through the string to extract all other tokens
	from=(char*)malloc(strlen(token)+1);
	strcpy(from,token);
	token = strtok(NULL, "\27");
	
	to=(char*)malloc(strlen(token)+1);
	strcpy(to,token);

	fullpath(f_from,from);
	fullpath(f_to,to);

	printf("%s\n",f_from);
	printf("%s\n",f_to);

	res._res = rename(f_from, f_to);
	
	if (res._res == -1)
		res._errno= -errno;
	sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);


	free(from);
	free(to);
	return 0;
}

//static int xmp_link(const char *from, const char *to)
static int xmp_link(int sockfd, struct sockaddr_in *cliaddr)
{
	_val res;
	char path[2*MAXLINE];
	char *from;
	char *to;
	char f_from[MAXLINE];
	char f_to[MAXLINE];

	int len = sizeof(struct sockaddr_in);
	size_t n=0;	
	//get path
	n = recvfrom(sockfd,path,2*MAXLINE,MSG_WAITALL,cliaddr,&len);
	path[n]='\0';	

	char * token = strtok(path, "\27");
	// loop through the string to extract all other tokens
	from=(char*)malloc(strlen(token)+1);
	strcpy(from,token);
	token = strtok(NULL, "\27");
	
	to=(char*)malloc(strlen(token)+1);
	strcpy(to,token);
	printf("%s\n",from);
	printf("%s\n",to);

	fullpath(f_from,from);
	fullpath(f_to,to);

	res._res = link(f_from, f_to);

	if (res._res == -1)
		res._errno= -errno;
	sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);


	return 0;
}

//static int xmp_chmod(const char *path, mode_t mode)
static int xmp_chmod(int sockfd, struct sockaddr_in *cliaddr)
{
	char path[MAXLINE];
	char f_path[MAXLINE];
	size_t n=0;
	int len = sizeof(struct sockaddr_in);
	mode_t mode;
	_val res;
	
	//get path
	n = recvfrom(sockfd,path,MAXLINE,MSG_WAITALL,cliaddr,&len);
	path[n]='\0';	
	recvfrom(sockfd,&mode,sizeof(mode_t),MSG_WAITALL,cliaddr,&len);
	fullpath(f_path,path);

	res._res = chmod(f_path, mode);
	if (res._res == -1)
		res._errno = -errno;

	sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);
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

//static int xmp_utimens(const char *path, const struct timespec ts[2])
static int xmp_utimens(int sockfd, struct sockaddr_in *cliaddr)
{
	_val res;
    struct timespec ts[2];
	struct timeval tv[2];
    
    char path[MAXLINE];
    char f_path[MAXLINE];
    size_t n=0;
    int len = sizeof(struct sockaddr_in);
    mode_t mode;

    //get path
    n = recvfrom(sockfd,path,MAXLINE,MSG_WAITALL,cliaddr,&len);
    path[n]='\0';
    fullpath(f_path,path);
    
    recvfrom(sockfd,ts,sizeof(struct timespec)*2,MSG_WAITALL,cliaddr,&len);

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res._res = utimes(f_path, tv);
	if (res._res == -1)
		res._errno=-errno;
    
	sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);
	return 0;
}

//static int xmp_open(const char *path, struct fuse_file_info *fi)
static int xmp_open(int sockfd, struct sockaddr_in *cliaddr)
{
	_val res;
	struct fuse_file_info fi;
	
	char path[MAXLINE];
	char f_path[MAXLINE];

	int len = sizeof(struct sockaddr_in);
	size_t n=0;	

	//get path
	n = recvfrom(sockfd,path,MAXLINE,MSG_WAITALL,cliaddr,&len);
	path[n]='\0';	

	fullpath(f_path,path);	
	recvfrom(sockfd,&fi,sizeof(struct fuse_file_info),MSG_WAITALL,cliaddr,&len);

	res._res = open(f_path, fi.flags);
	if (res._res == -1)
		res._errno = -errno;
		sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);
		return res._res;

    
	sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);
	sendto(sockfd,f_path,sizeof(MAXLINE),MSG_CONFIRM,cliaddr,len);


	close(res._res);
	return 0;
}

//static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
//		    struct fuse_file_info *fi)
static int xmp_read(int sockfd, struct sockaddr_in *cliaddr)
{
	int fd;
	_val res;

	//(void) fi;

	char path[MAXLINE];
	char f_path[MAXLINE];

	int len = sizeof(struct sockaddr_in);
	size_t n=0;	
	struct _args arg;	
	char *buf;

	//get path
	n = recvfrom(sockfd,path,MAXLINE,MSG_WAITALL,cliaddr,&len);
	path[n]='\0';	

	fullpath(f_path,path);
	recvfrom(sockfd,&arg,sizeof(struct _args),MSG_WAITALL,cliaddr,&len);
	


	fd = open(f_path, O_RDONLY);
	if (fd == -1){
		res._res = -1;
		res._errno= -errno;
		sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);
		return -errno;
	}
	buf = (char*)malloc(arg._size);	
	res._res = pread(fd, buf, arg._size, arg._offset);
	if (res._res == -1){
		res._errno = -errno;
		sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);
		free(buf);
		return -errno;
	}
	sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);
	sendto(sockfd,buf,arg._size,MSG_CONFIRM,cliaddr,len);
	
	close(fd);
	free(buf);
	return res._res;
}

//static int xmp_write(const char *path, const char *buf, size_t size,
//		     off_t offset, struct fuse_file_info *fi)
static int xmp_write(int sockfd, struct sockaddr_in *cliaddr)
{
	int fd;
	_val res;

	//(void) fi;
	

	char path[MAXLINE];
	char f_path[MAXLINE];

	int len = sizeof(struct sockaddr_in);
	size_t n=0;	
	struct _args arg;	

	//get path
	n = recvfrom(sockfd,path,MAXLINE,MSG_WAITALL,cliaddr,&len);
	path[n]='\0';	

	fullpath(f_path,path);
	recvfrom(sockfd,&arg,sizeof(struct _args),MSG_WAITALL,cliaddr,&len);
	arg._data = malloc(arg._size);
	recvfrom(sockfd,arg._data,arg._size,MSG_WAITALL,cliaddr,&len);


	fd = open(f_path, O_WRONLY);

	if (fd == -1){
		res._res = -1;
		res._errno = -errno;
		sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);
		return -errno;
	}

	res._res = pwrite(fd, arg._data, arg._size, arg._offset);
	if (res._res == -1){
		res._errno = -errno;
		sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);
		return -errno;
	}
	
	sendto(sockfd,&res,sizeof(_val),MSG_CONFIRM,cliaddr,len);
	close(fd);
	free(arg._data);
	return res._res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;

	res = statvfs(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

//static int xmp_release(const char *path, struct fuse_file_info *fi)
static int xmp_release(int sockfd, struct sockaddr_in *cliaddr)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	
	char path[MAXLINE];
	char f_path[MAXLINE];

	int len = sizeof(struct sockaddr_in);
	size_t n=0;	

	//get path
	n = recvfrom(sockfd,path,MAXLINE,MSG_WAITALL,cliaddr,&len);
	path[n]='\0';	

	fullpath(f_path,path);
    sendto(sockfd,f_path,MAXLINE,MSG_CONFIRM,cliaddr,len);
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
	
	if(argc > 1)
		strcpy(ROOTPATH,argv[1]);
	
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
	
	while(1){

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
			case READDIR:
			xmp_readdir(sockfd,&cliaddr);
			break;
			case MKDIR:
			xmp_mkdir(sockfd,&cliaddr);
			break;	
			case UNLINK:
			xmp_unlink(sockfd,&cliaddr);
			break;
			case RMDIR:
			xmp_rmdir(sockfd,&cliaddr);
			break;
			case SYMLINK:
			xmp_symlink(sockfd,&cliaddr);
			break;
			case RENAME:
			xmp_rename(sockfd,&cliaddr);
			break;
			case LINK:
			xmp_link(sockfd,&cliaddr);
			break;
			case OPEN:
			xmp_open(sockfd,&cliaddr);
			break;
			case READ:
			xmp_read(sockfd,&cliaddr);
			break;
			case WRITE:
			xmp_write(sockfd,&cliaddr);
			break;
			case CHMOD:
			xmp_chmod(sockfd,&cliaddr);
			break;
			case MKNOD:
			xmp_mknod(sockfd,&cliaddr);
			break;
            case RELEASE:
            xmp_release(sockfd,&cliaddr);
            break;
            case UTIME:
            xmp_utimens(sockfd,&cliaddr);
            break;
		}
	}
	close(sockfd);
	return 0;

}
