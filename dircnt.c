/**
 *
 * dircnt.c - a fast file-counting program.
 *
 * Written 2015-02-06 by Christopher Schultz as a programming demonstration
 * for a StackOverflow answer:
 * https://stackoverflow.com/questions/1427032/fast-linux-file-count-for-a-large-number-of-files/28368788#28368788
 *
 * Please see the README.md file for compilation and usage instructions.
 *
 * Thanks to FlyingCodeMonkey, Gary R. Van Sickle, and Jonathan Leffler for
 * various suggestions and improvements to the original code. Any additional
 * contributors can be found by looking at the GitHub revision history from
 * this point forward..
 */
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
#if PREFER_STAT || !defined ( _DIRENT_HAVE_D_TYPE )
    struct stat statbuf;     /* buffer for stat() info */
#endif

#ifdef DEBUG
    fprintf(stderr, "Opening dir %s\n", path);
#endif
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
#if ( defined ( _DIRENT_HAVE_D_TYPE ) && !PREFER_STAT)
      if(DT_DIR == ent->d_type) {
#else
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

#ifdef DEBUG
    fprintf(stderr, "Closing dir %s\n", path);
#endif
    closedir(dir);
}

int main(int argc, char *argv[]) {
    struct filecount counts;
    char *dir;
    counts.files = 0;
    counts.dirs = 0;
    if(argc > 1)
        dir = argv[1];
    else
        dir = ".";

#ifdef DEBUG
#if PREFER_STAT
    fprintf(stderr, "Compiled with PREFER_STAT. Using stat()\n");
#elif defined ( _DIRENT_HAVE_D_TYPE )
    fprintf(stderr, "Using dirent.d_type\n");
#else
    fprintf(stderr, "Don't have dirent.d_type, falling back to using stat()\n");
#endif
#endif

    count(dir, &counts);

    /* If we found nothing, this is probably an error which has already been printed */
    if(0 < counts.files || 0 < counts.dirs) {
        printf("%ld files and %ld directories in %s\n", counts.files, counts.dirs, dir);
    }

    return 0;
}

