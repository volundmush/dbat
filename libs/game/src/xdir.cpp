#include "dbat/game/xdir.h"


int xdir_scan(char *dir_name, struct xap_dir *xapdirp) {
  xapdirp->total = scandir(dir_name,&(xapdirp->namelist),0,alphasort);
  xapdirp->current = 0;

  return(xapdirp->total);
}

char *xdir_get_name(struct xap_dir *xd,int i) {
  return xd->namelist[i]->d_name;
}

char *xdir_get_next(struct xap_dir *xd) {
  if(++(xd->current) >= xd->total) {
    return NULL;
  }
  return xd->namelist[xd->current-1]->d_name;
}


void xdir_close(struct xap_dir *xd) {
  int i;
  for(i=0;i < xd->total;i++) {
    free(xd->namelist[i]);
  }
  free(xd->namelist);
  xd->namelist = NULL;
  xd->current = xd->total = -1;
}

int xdir_get_total(struct xap_dir *xd) {
  return xd->total;
}

int insure_directory(char *path, int isfile) {
  char *chopsuey = strdup(path);
  char *p;
  char *temp;
  struct stat st;

  extern int errno;

  /* if it's a file, remove that, we're only checking dirs; */
  if(isfile) {
    if(!(p=strrchr(path,'/'))) {
      free(chopsuey);
      return 1;
    }
    *p = '\0';
  }


  /* remove any trailing /'s */

  while(chopsuey[strlen(chopsuey)-1] == '/') {
    chopsuey[strlen(chopsuey) -1 ] = '\0';
  }

  /* check and see if it's already a dir */


    if(!stat(chopsuey,&st) && S_ISDIR(st.st_mode)) {
    free(chopsuey);
    return 1;
  }

  temp = strdup(chopsuey);
  if((p = strrchr(temp,'/')) != NULL) {
    *p = '\0';
  }
  if(insure_directory(temp,0) &&

          !mkdir(chopsuey, S_IRUSR | S_IWRITE | S_IEXEC | S_IRGRP | S_IXGRP |
                           S_IROTH | S_IXOTH)) {
    free(temp);
    free(chopsuey);
    return 1;
  }

  if(errno == EEXIST &&
          !stat(temp,&st)
     && S_ISDIR(st.st_mode)) {
    free(temp);
    free(chopsuey);
    return 1;
  } else {
    free(temp);
    free(chopsuey);
    return 1;
  }
}