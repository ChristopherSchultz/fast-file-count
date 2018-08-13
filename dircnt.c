/**
 * dircnt.c - a fast file-counting program.
 *
 * Written 2015-02-06 by Christopher Schultz as a programming demonstration
 * for a StackOverflow answer:
 * https://stackoverflow.com/questions/1427032/fast-linux-file-count-for-a-large-number-of-files/28368788#28368788
 *
 * This code is licensed under the Apache License 2.0. Please read the file
 * LICENSE for more information.
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

#define EXIT_REACHED_LIMIT 0x01

/* A custom structure to hold separate file and directory counts */
struct filecount {
  unsigned long dirs;
  unsigned long files;
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
    struct stat statbuf;     /* buffer for stat() info. A call to lstat() might be
                                required even if _DIRENT_HAVE_D_TYPE is true
                                because ent->d_type might be DT_UNKNOWN */
    int isdir;               /* flag for a directory entry being a directory */

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

      isdir = 0; /* reset isdir flag */
#ifdef DEBUG
      fprintf(stderr, "Considering %s%c%s\n", path, PATH_SEPARATOR, ent->d_name);
#endif /* DEBUG */

/* Use dirent.d_type if present, otherwise use stat() */
#if ( defined ( _DIRENT_HAVE_D_TYPE ) && !PREFER_STAT)
      if(DT_UNKNOWN == ent->d_type) {
          /* Must perform lstat() anyway */
#ifdef DEBUG
          fprintf(stderr, "Dirent type is DT_UNKNOWN, must perform lstat()\n");
#endif /* DEBUG */
          sprintf(subpath, "%s%c%s", path, PATH_SEPARATOR, ent->d_name);
          if(lstat(subpath, &statbuf)) {
              perror(subpath);
              return;
          }
          if(S_ISDIR(statbuf.st_mode)) {
#ifdef DEBUG
              fprintf(stderr, "Determined %s is a directory via lstat(1)\n", subpath);
#endif /* DEBUG */
             isdir = 1;
          }
      } else if(DT_DIR == ent->d_type) {
#ifdef DEBUG
          fprintf(stderr, "Determined %s%c%s is a directory via dirent\n", path, PATH_SEPARATOR, ent->d_name);
#endif /* DEBUG */
          isdir = 1;
      }
#else
      sprintf(subpath, "%s%c%s", path, PATH_SEPARATOR, ent->d_name);
      if(lstat(subpath, &statbuf)) {
          perror(subpath);
          return;
      }

      if(S_ISDIR(statbuf.st_mode)) {
#ifdef DEBUG
          fprintf(stderr, "S_ISDIR=%d, mode bits=%x\n", S_ISDIR(statbuf.st_mode), statbuf.st_mode);
          fprintf(stderr, "Determined %s is a directory via lstat(2)\n", subpath);
#endif /* DEBUG */
          isdir = 1;
      }
#endif /* if defined _DIRENT_HAVE_D_TYPE, etc. */

#ifdef DEBUG
      fprintf(stderr, "name=%s, isdir=%d\n", ent->d_name, isdir);
#endif

      if(isdir) {
          /* Skip "." and ".." directory entries... they are not "real" directories */
          if(0 == strcmp("..", ent->d_name) || 0 == strcmp(".", ent->d_name)) {
/*              fprintf(stderr, "This is %s, skipping\n", ent->d_name); */
          } else {
              if(ULONG_MAX == counts->dirs) {
                  fprintf(stderr, "Reached maximum number of directories to count (%lu) after %lu files\n", counts->dirs, counts->files);
                  exit(EXIT_REACHED_LIMIT);
              }
              sprintf(subpath, "%s%c%s", path, PATH_SEPARATOR, ent->d_name);
              counts->dirs++;
              count(subpath, counts);
          }
      } else {
          if(ULONG_MAX == counts->files) {
            fprintf(stderr, "Reached maximum number of files to count (%lu) after %lu directories\n", counts->files, counts->dirs);
            exit(EXIT_REACHED_LIMIT);
          }

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
    fprintf(stderr, "Compiled with PREFER_STAT. Using lstat()\n");
#elif defined ( _DIRENT_HAVE_D_TYPE )
    fprintf(stderr, "Using dirent.d_type\n");
#else
    fprintf(stderr, "Don't have dirent.d_type, falling back to using lstat()\n");
#endif
#endif

    count(dir, &counts);

    /* If we found nothing, this is probably an error which has already been printed */
    if(0 < counts.files || 0 < counts.dirs) {
        printf("%s contains %lu files and %lu directories\n", dir, counts.files, counts.dirs);
    }

    return 0;
}

