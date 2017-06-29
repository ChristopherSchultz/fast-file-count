#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>

#if defined(WIN32) || defined(_WIN32) 
#define PATH_SEPARATOR '\\' 
#else
#define PATH_SEPARATOR '/' 
#endif

/* A custom structure to hold separate file and directory counts */
struct filecount {
  long dirs;
  long files;
};

/*
 * counts the number of files and directories in the specified directory.
 *
 * path - relative pathname of a directory whose files should be counted
 * counts - pointer to struct containing file/dir counts
 */
void count(char *path, struct filecount *counts) {
    DIR *dir;                /* dir structure we are reading */
    struct dirent *ent;      /* directory entry currently being processed */
    char subpath[PATH_MAX];  /* buffer for building complete subdir and file names */
    /* Some systems don't have dirent.d_type field; we'll have to use stat() instead */
#if !defined ( _DIRENT_HAVE_D_TYPE )
    struct stat statbuf;     /* buffer for stat() info */
#endif

/* fprintf(stderr, "Opening dir %s\n", path); */
    dir = opendir(path);

    /* opendir failed... file likely doesn't exist or isn't a directory */
    if(NULL == dir) {
        perror(path);
        return;
    }

    while((ent = readdir(dir))) {
      if (strlen(path) + 1 + strlen(ent->d_name) > PATH_MAX) {
          fprintf(stdout, "path too long (%ld) %s%c%s", (strlen(path) + 1 + strlen(ent->d_name)), path, PATH_SEPARATOR, ent->d_name);
          return;
      }

/* Use dirent.d_type if present, otherwise use stat() */
#if defined ( _DIRENT_HAVE_D_TYPE )
/* fprintf(stderr, "Using dirent.d_type\n"); */
      if(DT_DIR == ent->d_type) {
#else
/* fprintf(stderr, "Don't have dirent.d_type, falling back to using stat()\n"); */
      sprintf(subpath, "%s%c%s", path, PATH_SEPARATOR, ent->d_name);
      if(lstat(subpath, &statbuf)) {
          perror(subpath);
          return;
      }

      if(S_ISDIR(statbuf.st_mode)) {
#endif
          /* Skip "." and ".." directory entries... they are not "real" directories */
          if(0 == strcmp("..", ent->d_name) || 0 == strcmp(".", ent->d_name)) {
/*              fprintf(stderr, "This is %s, skipping\n", ent->d_name); */
          } else {
              sprintf(subpath, "%s%c%s", path, PATH_SEPARATOR, ent->d_name);
              counts->dirs++;
              count(subpath, counts);
          }
      } else {
          counts->files++;
      }
    }

/* fprintf(stderr, "Closing dir %s\n", path); */
    closedir(dir);
}

int main(int argc, char *argv[]) {
    struct filecount counts;
    counts.files = 0;
    counts.dirs = 0;
    count(argv[1], &counts);

    /* If we found nothing, this is probably an error which has already been printed */
    if(0 < counts.files || 0 < counts.dirs) {
        printf("%s contains %ld files and %ld directories\n", argv[1], counts.files, counts.dirs);
    }

    return 0;
}

