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

#define USE_STAT  256
#define USE_LSTAT 257
#define BUFF_SIZE 1024

/* only absolute pathnames allowed, sorry*/

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
	 
	 DIR *dp = NULL;
	 strcpy(buff, path);
	 char *end_ptr = buff;
	 
	 if (NULL == (dp = opendir(path)) )
     {
		
        printf("\n Cannot open Input directory [%s]\n",path);
        return ENOTDIR;
     }
     
     else
     {
		//printf("\n name[%s]\n",name);
		//printf("\n name[%s]\n",buff); 
        if(buff[strlen(buff)-1]=='/')
        {
			//printf("\n bfr cpy name[%s]\n",buff);
			
            end_ptr += strlen(buff);
            strcpy(end_ptr, name);
            int z = strlen(buff);
			buff[z] = '/';
			++z;
			buff[z] = '\0';
			//printf("\n aftr cpy buff[%s]\n",buff);
        }
        else
        {
			
			int z = strlen(buff);
			buff[z] = '/';
			end_ptr += strlen(buff);
			strcpy(end_ptr, name);
			z = strlen(buff);
			buff[z] = '/';
			++z;
			buff[z] = '\0';
        }

        //printf("\n Creating a new directory [%s]\n", buff);
        mkdir(buff,S_IRWXU|S_IRWXG|S_IRWXO);
        
        strcpy(new_path, buff);
		
        return EOK;
	}
}


char* get_dir_name(const char* dir_path) 
{
    char *buf = NULL;
    int count = 0, has_slash = 0;
    int i = strlen(dir_path) - 1;
    if (dir_path[i] == '/') 
    {
		--i;
		has_slash = 1;
	}
    while ((i >= 0) && (dir_path[i] != '/'))
    {
		--i;
		++count;
	}
	buf = (char*) malloc (count* sizeof(char));
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
	destFD = open(dst,O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | 
	S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
 
	if(destFD == -1)
	{
		printf("\nError opening file %s \n",dst);
		return EBADMD;
	}
 
	/*Start data transfer from src file to dest file till it reaches EOF*/
	while((nb_read = read(srcFD,buff,BUFF_SIZE)) > 0)
	{
		if(write(destFD,buff,nb_read) != nb_read)
			return EBADMD;
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
 
	return EOK;
}

int rec_dir_cp(char* source, char* dest, char* name)
{
	
	char new_dest[PATH_MAX];
	int err = make_new_dir(dest, name, new_dest);
	
	if (err != EOK) return err;
	DIR *dir = opendir(source); //Assuming absolute pathname here.
    if (dir == NULL) return ENOTDIR;
    
    char Path[PATH_MAX], *EndPtr = Path;
    struct dirent *e;
    strcpy(Path, source);  
    //printf("Path %s\n", Path);
   
    EndPtr += strlen(Path); 
    
    while((e = readdir(dir)) != NULL) //Iterates through the entire directory.
    {  
        struct stat info;     
		char f[strlen(e->d_name)* sizeof(char) + strlen(Path) + 3];
		sprintf(f, "%s/%s", Path, e->d_name);
        //printf("\n rdc: let's see what %s file hides\n", f);      
        if(!stat(f, &info))
        {
			char e_name[PATH_MAX];
			strcpy(e_name, e->d_name);
			e_name[strlen(e_name)] = '\0';
			
            if(is_dir(f, 256)) //Are we dealing with a directory?
            { 
				if (e_name[0] != '.')
					rec_dir_cp(f, new_dest, e_name); //Calls this function AGAIN, this time with the sub-name. 
            } 
            
            else if (S_ISREG(info.st_mode) )//Or did we find a regular file?
            { 
                char ff[PATH_MAX];
                //printf("\ndest directory %s\n ",new_dest);
                sprintf(ff, "%s%s", new_dest, e_name);
				//printf("\ndest %s\n", ff);
                if ((e_name)[0] != '.')
					file_to_file(f, ff);
			}
			
		}
    }
    closedir(dir);
    return EOK;
}

int main(int argc, char** argv) {
	if (argc < 3) {
		printf("not enough args\n");
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
	
