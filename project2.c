#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>

// Superblock
typedef struct{
	unsigned short isize;
	unsigned short fsize;
	unsigned short nfree;
	unsigned short free[100];
	unsigned short ninode;
	unsigned short inode[100];
	char flock;
	char ilock;
	char fmod;
	unsigned short time[2];
} superblock;

// iNode
typedef struct {
	unsigned short flags;
	char nlinks;
	char uid;
	char gid;
	char size0;
	unsigned short size1;
	unsigned short addr[8];
	unsigned short actime[2];
	unsigned short modtime[2];
} inode;

// Function declarations to remove annoying compilation warnings
int ckFileType( char * in, int fd );
int fileSize( char * in , int fd );
int mount( char * path );
void printHelp();
inode getInode( int n, int fd);
char * toLower(char * str);
void toggleDebug(char * arg);


// Debug state. 0=off, 1=on
int debug;


// Main function that contains the input loop
int main(int argc, char *argv[]){

	if (argc > 1){
		if (strcmp(argv[1], "-d") == 0){ debug=1; }else{ debug=0; }
	}

	if (debug){ printf("|Running in debug mode.\n");}

	char input[100]; // command input buffer
	int fd = -1; // file mounted to be r/w from/to

	// Input execution loop:
	while(1){
		// get input into buffer and null terminate it
		printf(">");
		fgets(input, 100, stdin);
		input[ strlen(input) - 1 ] = '\0';

		// If there is any input text:
		if (strlen(input) != 0){
			// Get input and tokenize it into args[] array
			char delim[] = " \n";
			int i = 0;
			char * args[20];
      args[i] = toLower(strtok(input, delim));
      while(args[i] != NULL){args[++i]=strtok(NULL,delim);}

			// execute command at arg[0] with proper arguments
			if       (strcmp(args[0],"mount"     ) == 0){ fd = mount (args[1]     );
			}else if (strcmp(args[0],"ckfiletype") == 0){ ckFileType (args[1] , fd);
			}else if (strcmp(args[0],"filesize"  ) == 0){ fileSize   (args[1] , fd);
			}else if (strcmp(args[0],"h"         ) == 0 ||
								strcmp(args[0],"help"      ) == 0){	printHelp();
			}else if (strcmp(args[0],"d"         ) == 0 ||
								strcmp(args[0],"debug"     ) == 0){ toggleDebug(args[1]);
			}else if (strcmp(args[0],"q"         ) == 0){
				printf(" Exiting...\n");
				if (fd != -1){ close(fd); }
				exit(0);
			}else{
				printf(" Invalid command: \"%s\"\n", args[0]);
				printf(" Enter \"h\" or \"help\" for help.\n");
			}
		}
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// Utility functions:
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Get iNode n from filesystem fd
inode getInode( int n, int fd){
	inode node;
	if (fd == -1){
		printf(" Error getting inode: No filesystem mounted.\n");
		return node;
	}
	if (debug){ printf("|Getting inode...\n"); }
	if (debug){ printf("|Seeking to %d\n", 512*2 + (n-1)*32); }
	lseek(fd, 512 * 2 + (n-1) * 32, SEEK_SET);
	if (debug){ printf("|Reading from filesystem to node struct...\n"); }
	read(fd, &node, sizeof(node));
	if (debug){ printf("|Got iNode successfully.\n"); }
	return node;
}

// Take a string and turn it to lower case
char * toLower(char * str){
	int i = 0;
	if (str == NULL){ return str; }
	while( str[i] ) {
		str[i] = (char)tolower(str[i]);
		i++;
	}
	return str;
}


///////////////////////////////////////////////////////////////////////////////
// Command functions:
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Help / H
// Print list of available commands
void printHelp(){
	printf(" Available commands:\n");
	printf("  ckfiletype X - display info about file with inode #X\n");
	printf("  d / debug X  - set debug to X (1/0) or toggle if no X\n");
	printf("  filesize X   - display file size of file with inode #X\n");
	printf("  h / help     - display help\n");
	printf("  mount X      - mount filesystem at path X\n");
	printf("  q            - exit program\n");
	printf("\n");
	printf(" Execute with \"-d\" to enable debug from beginning.\n");
}

///////////////////////////////////////////////////////////////////////////////
// Debug / d
// Enable or disable debug
void toggleDebug(char * arg){
	if (arg == NULL){
		debug = !debug;
	}else{
		debug = atoi(arg);
	}
	if(debug){ printf(" Debug enabled.\n");
	}else{     printf(" Debug disabled.\n");}
}

///////////////////////////////////////////////////////////////////////////////
// Mount
// Open filesystem and get file descriptor
int mount( char * path ){
	if (path == NULL){
		printf(" Error mounting: no path specified\n");
		return -1;
	}
	printf(" Mounting filesytem... ");
	int fd = open(path, 2);
	if (fd == -1){
		printf("Error mounting file: \"%s\". ", path);
		printf("No file mounted.\n");
		return -1;
	}
	printf("filesystem mounted successfully.\n");
	return fd;
}

///////////////////////////////////////////////////////////////////////////////
// ckfiletype
// Check inode and display info about it
int ckFileType( char * in, int fd ){
	if (fd == -1){
		printf(" Error checking file type: no filesystem mounted.\n");
		return -1;
	}
	if (in == NULL){
		printf(" Error checking file type: no inode number provided.\n");
		return -1;
	}

	if (debug){ printf("|Finding iNode %s...\n", in); }
	int n = 0;
	if (in){ n = atoi(in); }
	if (n < 1 ){
		printf(" Error checking file type: invalid inode \"%d\".\n", n);
		return -1;
	}

	inode node = (inode) getInode( n, fd );
	if (debug){ printf("|Getting flags from iNode...\n"); }
	unsigned short flags = node.flags;
  if (debug){ printf("|Read flags as:\n"); }
	if (debug){ printf("| dec: %d\n", flags);}
	if (debug){ printf("| hex: %x\n", flags);}
	if (debug){ printf("| oct: %o\n", flags);}
	int allocated = (flags & (1 << 15)) >> 15;
	if (!allocated){
		printf(" Inode %d: Not Allocated.\n", n);
		return 0;
	}

	int fileType = (flags & (11 << 13)) >> 13;
	printf(" Inode %d:\n", n);
	printf("  File type: ");
	switch (fileType){
		case 0: printf("Plain File\n");
			break;
		case 1: printf("Char-Type Special File\n");
			break;
		case 2: printf("Directory\n");
			break;
		case 3: printf("Block-Type Special File\n");
	}

	int isLarge = (flags & (1 << 12)) >> 12;
	switch( isLarge ){
		case 0: printf("  Small File (<4kB)\n");
			break;
		case 1: printf("  Large File (>4kB)\n");
	}

	fileSize( in, fd );

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// filesize
// Check inode and display its file's size
int fileSize( char * in , int fd ){

	if (fd == -1){
		printf(" Error checking file size: no filesystem mounted.\n");
		return -1;
	}

	if (in == NULL){
		printf(" Error checking file size: no inode number provided.\n");
		return -1;
	}

	if (debug){ printf("|Finding iNode %s...\n", in); }
	int m = 0;
	if (in){ m = atoi(in); }
	if (m < 1){
		printf(" Error checking file size: invalid inode \"%d\".\n", m);
		return -1;
	}

	inode node = (inode) getInode( m, fd );
	if (debug){ printf("| Size 0 = %d\n", (int)(node.size0)); }
	if (debug){ printf("| Size 1 = %d\n", (int)(node.size1)); }

	int size0 = ((int)(node.size0)) << 16;
	int size1 = (int)(node.size1);
	int size = size0 + size1;

	printf(" Size of file: %d bytes\n",size);
}