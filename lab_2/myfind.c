#include "include/apue.h"
#include <dirent.h>
#include <limits.h>
#include <fcntl.h>

/* function type that is called for each filename */
typedef int Myfunc(const char *, const struct stat *, int);

static Myfunc myfuncCount, myfuncContentCompare, myfuncNameCompare;
static int myftw(char *, Myfunc *);
static int dopath(Myfunc *);
static char * getAbsolutePath(const char *pathname, char *absolutepath);


static long nreg, ndir, nblk, nchr, nfifo, nslink, nsock, ntot, nsize4096;
static long filesize;
static char *filebuffsize, *comparedfilebuffsize, *absolutepath;
static size_t abso_pathlen;
static int global_argc;
static char **global_argv;
static int filecnt;

int main(int argc, char *argv[])
{
	int ret;
	if (argc == 2)
	{
		ret = myftw(argv[1], myfuncCount); /* does it all */
		ntot = nreg + ndir + nblk + nchr + nfifo + nslink + nsock;
		if (ntot == 0)
			ntot = 1; /* avoid divide by 0; print 0 for all counts */
		printf("regular files  = %7ld, %5.2f %%,\n", nreg,
			   nreg * 100.0 / ntot);
		printf("whose length is not greater than 4096 = %7ld, %5.2f %%\n", nsize4096,
			   nsize4096 * 100.0 / nreg);
		printf("directories    = %7ld, %5.2f %%\n", ndir,
			   ndir * 100.0 / ntot);
		printf("block special  = %7ld, %5.2f %%\n", nblk,
			   nblk * 100.0 / ntot);
		printf("char special   = %7ld, %5.2f %%\n", nchr,
			   nchr * 100.0 / ntot);
		printf("FIFOs          = %7ld, %5.2f %%\n", nfifo,
			   nfifo * 100.0 / ntot);
		printf("symbolic links = %7ld, %5.2f %%\n", nslink,
			   nslink * 100.0 / ntot);
		printf("sockets        = %7ld, %5.2f %%\n", nsock,
			   nsock * 100.0 / ntot);
	}

	if (argc == 4 && strcmp(argv[2],"-comp") == 0){
		// Input Check
		struct stat buf;
		if (lstat(argv[3], &buf) < 0){
			err_quit("lstat error 000");
		}
		if (S_ISREG(buf.st_mode) == 0){
			err_quit("not a regular file!");
		}
		// Open file
		int fd;
		if ((fd = open(argv[3], O_RDONLY, FILE_MODE)) == -1){
			err_sys("open error");
		}
		filesize = buf.st_size;
		if ((filebuffsize = (char *)malloc(sizeof(char) * filesize)) == 0){
			err_sys("malloc failed");
		}
		if ((comparedfilebuffsize = (char *)malloc(sizeof(char) * filesize)) == NULL){
			err_sys("malloc failed");
		}
		// Read file
		if (read(fd, filebuffsize, filesize) != filesize){
			err_sys("read error");
		}
		close(fd);
		// Get absolute path
		absolutepath = path_alloc(&abso_pathlen);
		getAbsolutePath(argv[1], absolutepath);
		// Find
		printf("The files same as %s are:\n", argv[3]);
		ret = myftw(absolutepath, myfuncContentCompare);
		if (filecnt == 0){
			printf("No matching files found\n");
		}
	}

	if (argc >= 4 && strcmp(argv[2], "-name") == 0){
		global_argc = argc;
		global_argv = argv;
		static size_t len;
		absolutepath = path_alloc(&len);
		absolutepath = getAbsolutePath(argv[1], absolutepath);
		printf("Matching files:\n");
		ret = myftw(absolutepath, myfuncNameCompare);
		if (filecnt == 0){
			printf("No matching files found\n");
		}
	}
	exit(ret);
}

#define FTW_F 1	  /* file other than directory */
#define FTW_D 2	  /* directory */
#define FTW_DNR 3 /* directory that can't be read */
#define FTW_NS 4  /* file that we can't stat */

static char *fullpath; /* contains full pathname for every file */
static size_t pathlen;

static int /* we return whatever func() returns */
myftw(char *pathname, Myfunc *func)
{
	fullpath = path_alloc(&pathlen); /* malloc PATH_MAX+1 bytes */
									 /* ({Prog pathalloc}) */
	if (pathlen <= strlen(pathname))
	{
		pathlen = strlen(pathname) * 2;
		if ((fullpath = realloc(fullpath, pathlen)) == NULL)
			err_sys("realloc failed");
	}
	strcpy(fullpath, pathname);
	return (dopath(func));
}

static int					/* we return whatever func() returns */
dopath(Myfunc* func)
{
	struct stat		statbuf;
	struct dirent	*dirp;
	DIR				*dp;
	int				ret, n;

	if (lstat(fullpath, &statbuf) < 0)	/* stat error */
		return(func(fullpath, &statbuf, FTW_NS));

	if (S_ISDIR(statbuf.st_mode) == 0)	/* not a directory */
		return(func(fullpath, &statbuf, FTW_F));

	/*
	 * It's a directory.  First call func() for the directory,
	 * then process each filename in the directory.
	 */
	if ((ret = func(fullpath, &statbuf, FTW_D)) != 0)
		return(ret);

	n = strlen(fullpath);
	if (n + NAME_MAX + 2 > pathlen) {	/* expand path buffer */
		pathlen *= 2;
		if ((fullpath = realloc(fullpath, pathlen)) == NULL)
			err_sys("realloc failed");
	}
	if (fullpath[n - 1] != '/'){
		fullpath[n++] = '/';
		fullpath[n] = 0;
	}
	if ((dp = opendir(fullpath)) == NULL)	/* can't read directory */
		return(func(fullpath, &statbuf, FTW_DNR));

	while ((dirp = readdir(dp)) != NULL) {
		if (strcmp(dirp->d_name, ".") == 0  ||
		    strcmp(dirp->d_name, "..") == 0)
				continue;		/* ignore dot and dot-dot */
		strcpy(&fullpath[n], dirp->d_name);	/* append name after "/" */
		if ((ret = dopath(func)) != 0)		/* recursive */
			break;	/* time to leave */
	}
	fullpath[n-1] = 0;	/* erase everything from slash onward */

	if (closedir(dp) < 0)
		err_ret("can't close directory %s", fullpath);
	return(ret);
}

// Get the absolute path
static char * 
getAbsolutePath(const char *pathname, char *absolutepath)
{
	// chdir : change work dir
	static size_t length;
	char *currentpath; //current workd dir
	currentpath = path_alloc(&length);
	if (getcwd(currentpath, length) == NULL){
		err_sys("getcwd error");
	}
	if (chdir(pathname) < 0){
		err_sys("chdir error");
	}
	if (getcwd(absolutepath, length) == NULL){
		err_sys("getcwd error");
	}
	// back to ori path
	if (chdir(currentpath) < 0){
		err_sys("chdir error");
	}
	return absolutepath;
}

//function 1 count the file
static int
myfuncCount(const char *pathname, const struct stat *statptr, int type)
{
	switch (type)
	{
	case FTW_F:
		switch (statptr->st_mode & S_IFMT)
		{
		case S_IFREG:
			nreg++;
			if (statptr->st_size <= 4096)
				nsize4096++;
			break;
		case S_IFBLK:
			nblk++;
			break;
		case S_IFCHR:
			nchr++;
			break;
		case S_IFIFO:
			nfifo++;
			break;
		case S_IFLNK:
			nslink++;
			break;
		case S_IFSOCK:
			nsock++;
			break;
		case S_IFDIR: /* directories should have type = FTW_D */
			err_dump("for S_IFDIR for %s", pathname);
		}
		break;
	case FTW_D:
		ndir++;
		break;
	case FTW_DNR:
		err_ret("can't read directory %s", pathname);
		break;
	case FTW_NS:
		err_ret("stat error for %s", pathname);
		break;
	default:
		err_dump("unknown type %d for pathname %s", type, pathname);
	}
	return 0;
}

//function 2 compare file
static int
myfuncContentCompare(const char *pathname, const struct stat *statptr, int type)
{
	if (type == FTW_F && S_ISREG(statptr->st_mode) && statptr->st_size == filesize){
		int fd;
		if ((fd = open(pathname, O_RDONLY,FILE_MODE)) == -1){
			return 0;
		}
		if (read(fd, comparedfilebuffsize, statptr->st_size) != statptr->st_size){
			err_sys("read error");
		}
		close(fd);
		if (strcmp(filebuffsize, comparedfilebuffsize) == 0){
			printf("%s\n", pathname);
			filecnt++;
		}
	}
	return 0;
}

//function 3
static int
myfuncNameCompare(const char *pathname, const struct stat *statptr, int type)
{
	if (type == FTW_F){
		// remove the dir;keep the filename
		int i, pos = 0;
		for (i = strlen(pathname) - 1;i >= 0;i--){
			if (pathname[i] == '/'){
				pos = i;
				break;
			}
		}
		pos++;
		const char *curfilename = pathname + pos;
		for (i = 3;i < global_argc;i++){
			if (strcmp(curfilename, global_argv[i]) == 0){
				printf("%s\n", pathname);
				filecnt++;
			}
		}
	}
	return 0;
}
