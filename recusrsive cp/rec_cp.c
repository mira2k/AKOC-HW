#include <stdio.h>
#include <limits.h>
#include <string.h> 
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define EOK     0
#define ENOARG  1
#define ENOTDIR 2

#define EMALLOC 3

#define EBADMD  4
#define ECHMOD  5

#define USE_STAT  256
#define USE_LSTAT 257
#define BUFF_SIZE 1024

#define WRITE_MODE O_WRONLY | O_CREAT | O_TRUNC
#define PERM_MODE_1 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH
#define PERM_MODE_2 S_IRWXU|S_IRWXG|S_IRWXO
/* only absolute pathnames are allowed, sorry*/

int is_dir(const char* path, int mode)
{
    struct stat st;
    switch(mode)
    {
    case USE_STAT:
        stat(path, &st);
        break;
    case USE_LSTAT:
        lstat(path, &st);
        break;
    default:
        exit(EBADMD);

    }
    return (int)((st.st_mode & S_IFMT) == S_IFDIR);
}

int make_new_dir (char* path, char* name, char* new_path)
{
     char buff[PATH_MAX];
     char *end_ptr;
     buff[0] = '\0';
     DIR *dp = NULL;
     strcpy(buff, path);
     buff[strlen(path)] = '\0';
     end_ptr = buff;
     
     if (NULL == (dp = opendir(path)) )
     {
        printf("\n Cannot open Input directory [%s]\n",path);
        closedir(dp);
        return ENOTDIR;
     }
     
     else
     { 
        if(buff[strlen(buff)-1]=='/')
        {
            /*printf("\n bfr cpy name[%s]\n",buff);*/
            
            end_ptr += strlen(buff);
            strcpy(end_ptr, name);
            int z = strlen(buff);
            buff[z] = '/';
            ++z;
            buff[z] = '\0';
            /*printf("\n aftr cpy buff[%s]\n",buff);*/
        }
        else
        {
            int z = strlen(buff);
            buff[z] = '/';
            buff[z + 1] = '\0';
            end_ptr += strlen(buff);
            strcpy(end_ptr, name);
            z = strlen(buff);
            buff[z] = '/';
            ++z;
            buff[z] = '\0';
        }

        /*printf("\n Creating a new directory [%s]\n", buff);*/
        mkdir(buff,PERM_MODE_2);
        
        strcpy(new_path, buff);
        
        closedir(dp);
        return EOK;
    }
}

char* get_dir_name(const char* dir_path) 
{
    char *buf = NULL;
    int count = 0, has_slash = 0;
    int len = 0;
    int i = strlen(dir_path) - 1;
    if (dir_path[i] == '/') 
    {
        --i;
        has_slash = 1;
        printf("has slash\n");
    }

    while ((i >= 0) && (dir_path[i] != '/'))
    {
        --i;
        ++count;
    }
    len = count;
    buf = (char*) malloc (len + 1);
    if (buf == NULL)
    {
        printf ("Bad malloc(((");
        return NULL;
    }
    count = 0;
    for (++i; i < strlen(dir_path) - has_slash; ++i)
    {
        buf[count] = dir_path[i];
        ++count;
    }
    buf[count] = '\0';
    return buf;
}

int file_to_file (const char* src, const char* dst) 
{
    int srcFD;
    int destFD;
    int nb_read;
    char *buff[BUFF_SIZE];
    srcFD = open(src,O_RDONLY);
 
    if(srcFD == -1)
    {
        printf("\nError opening file %s\n", src);
        return EBADMD;  
    }
    destFD = open(dst, WRITE_MODE, PERM_MODE_1);
 
    if(destFD == -1)
    {
        printf("\nError opening file %s \n",dst);
        return EBADMD;
    }
 
    /*Start data transfer from src file to dest file till it reaches EOF*/
    while((nb_read = read(srcFD,buff,BUFF_SIZE)) > 0)
    {
        if(write(destFD,buff,nb_read) != nb_read)
        {
            return EBADMD;
        }
    }
    
    if(nb_read == -1) 
    {
        printf("\nError in reading data from %s\n",src);
        return EBADMD;
    }
    
    if(close(srcFD) == -1) 
    {
        printf("\nError in closing file %s\n",src);
        return EBADMD;
    }
        
 
    if(close(destFD) == -1)
    {
        printf("\nError in closing file %s\n",dst);
        return EBADMD;
    }
    struct stat src_st;
    if (stat(src, &src_st) < 0)
        return ECHMOD;
    if (chmod(dst, src_st.st_mode) < 0)
        return ECHMOD;
    return 0;
    return EOK;
}

int handle_symlinks (char* f, char* ff, char* e_name)
{   
    struct stat info;
    char * link;
    int r_link;
    link = NULL;
    if (lstat(f, &info))
    {
        if (link != NULL) free(link);
        return EBADMD;
    }
    
    link = (char*)malloc(info.st_size + 1);
    if (link == NULL)
    {
        if (link != NULL) free(link);
        return EMALLOC;
    }
    
    r_link = readlink(f, link, info.st_size + 1);
    if (r_link < 0)
    {
    
        if (link != NULL) free(link);
        return EBADMD;
    }
    link[info.st_size] = '\0';
    
    if (symlink(link ,ff) == -1)
    {
        if (link != NULL)
        free(link);
        return EBADMD;
    }
    
    if (link != NULL)
        free(link);

    return EOK;
}

int rec_dir_cp(char* source, char* dest, char* name)
{
    
    char new_dest[PATH_MAX];
    int err;
    int control = EOK;
    char Path[PATH_MAX];
    struct dirent *e;
    err = make_new_dir(dest, name, new_dest);
    if (err != EOK) return err;
    DIR *dir = opendir(source);
    /*Assuming absolute pathname here.*/
    if (dir == NULL) return ENOTDIR;
    strcpy(Path, source);  
    /*printf("Path %s\n", Path);*/
    
    while (((e = readdir(dir)) != NULL) && control == EOK)
    /*Iterates through the entire directory.*/
    {  
        struct stat info;     
        char f[(strlen(e->d_name) + strlen(Path) + 3)];
        int res = EOK;
        sprintf(f, "%s/%s", Path, e->d_name);
        if(!stat(f, &info))
        {
            char e_name[PATH_MAX];
            strcpy(e_name, e->d_name);
            e_name[strlen(e_name)] = '\0';
            
            if(is_dir(f, 256))
            { 
                if (e_name[0] != '.')
                    res = rec_dir_cp(f, new_dest, e_name);
                control = res;
            } 
            
            else if (S_ISREG(info.st_mode) )
            { 
                char ff[PATH_MAX];
                sprintf(ff, "%s%s", new_dest, e_name);
                ff[strlen(ff)] = '\0';
                if ((e_name)[0] != '.') {
                    res = file_to_file(f, ff);
                }
                control = res;
            }
            else if (S_ISLNK(info.st_mode)) {
                char ff[PATH_MAX];
                sprintf(ff, "%s%s", new_dest, e_name);
                ff[strlen(ff)] = '\0';
                res = handle_symlinks(f, ff, e_name);
                control = res;
            }
        }
    }
    closedir(dir);
    return control;
}

int main(int argc, char** argv)
{
    if (argc < 3) 
    {
        printf("\nrcp: \nInput format: 'source directory path, destination directory path'\n");
        printf("\nNotice that only absolute pathnames are allowed\n");
        return ENOARG;
    }
    char* name = get_dir_name(argv[1]);
    if (name == NULL)
        return EMALLOC;
    int err = rec_dir_cp(argv[1], argv[2], name);
    free(name);
    
    if (err)
        return err;
    return 0;
}
