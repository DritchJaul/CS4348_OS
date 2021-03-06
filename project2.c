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

typedef struct {
	unsigned short inode;
	char name[14];
} directoryEntry;

typedef struct {
	unsigned short size;
	unsigned short addresses[100];
} chainBlock;

typedef struct {
	unsigned char contents[512];
} charBlock;




// Function declarations to remove annoying compilation warnings
int ckFileType( char * in );
int fileSize( char * in );
int mount( char * path );
void printHelp();
inode getInode( int n );
char * toLower(char * str);
void toggleDebug(char * arg);

int initfs( char * n1, char * n2);
int freeBlock (unsigned short n);
int allocateBlock();

int cpin( char * outsidePath, char * insidePath);


// Debug state. 0=off, 1=on
int debug;

char * filesystemName[100];
int fd; // file mounted to be r/w from/to
superblock super;



directoryEntry readDirectoryEntry( int block, int n){
	// if(debug){printf("|Reading %d bytes to file at block %d (%x)...\n", 16, block, 512 * block + 16*n);}
	directoryEntry file = { 0, "" };
	fseek(fd, 512 * block + 16*n, SEEK_SET);
	// read(fd, &file, 16);
	fread(&file, 16, 1, fd);
	return file;
}


charBlock readCharBlock(int n){
	charBlock buffer;
	fseek(fd, 512 * n, SEEK_SET);
	// read(fd, &buffer, 512);
	fread(&buffer, 512, 1, fd);
	return buffer;
}

chainBlock readChainBlock(int n){
	if (debug){ printf("|Reading Chainblock...\n");}
	chainBlock buffer;
	if (debug){ printf("| Seeking to %d (%x)...\n", n, 512*n);}
	fseek(fd, 512 * n, SEEK_SET);
	if (debug){ printf("| Freading...\n");}
	int error = fread(&buffer, sizeof(buffer), 1, fd);
	if (debug){ printf("| Fread error = %d\n", error);}
	if (debug){ printf("|Chainblock size = %d, address[0] = %d\n", buffer.size, buffer.addresses[0]);}

	return buffer;
}


void readSuperblock(){
	fseek(fd, 512, SEEK_SET);
	// read(fd, &super, sizeof(super));
	fread(&super, sizeof(super), 1, fd);
}


void writeChainBlock(int n, chainBlock buffer){
	if(debug){printf("|Writing %d bytes to file at block %d (%x)...", sizeof(buffer), n, 512 * n);}
	fseek(fd, 512 * n, SEEK_SET);
	// write(fd, &buffer, sizeof(buffer));
	int error = fwrite(&buffer, sizeof(buffer), 1, fd);
	if(debug){printf(" fwrite returned: %d\n",error );}

}


void writeCharBlock(int n, charBlock buffer){
	if(debug){printf("|Writing %d bytes to file at block %d (%x)...", sizeof(buffer), n, 512 * n);}
	fseek(fd, 512 * n, SEEK_SET);
	// write(fd, &buffer, 512);
	int error = fwrite(&buffer, 512, 1, fd);
	if(debug){printf(" fwrite returned: %d\n",error );}
}

void writeSuperblock(){
	if(debug){printf("|Writing %d bytes to the superblock of file...", sizeof(super));}
	fseek(fd, 512, SEEK_SET);
	// write(fd, &super, sizeof(super));
	int error = fwrite(&super, sizeof(super), 1, fd);
	if(debug){printf(" fwrite returned: %d\n",error );}
}

void writeInode( int n, inode node ){
	if(debug){printf("|Writing %d bytes to file at block %d (%x)...", sizeof(node), n, 512 * 2 + (n-1) * 32);}
	fseek(fd, 512 * 2 + (n-1) * 32, SEEK_SET);
	// write(fd, &node, sizeof(node));
	int error = fwrite(&node, sizeof(node), 1, fd);
	if(debug){printf(" fwrite returned: %d\n",error );}
}

void writeDirectoryEntry( int block, int n, directoryEntry entry){
	if(debug){printf("|Writing %d bytes to file at block %d (%x)...", sizeof(entry), block, 512 * block + 16*n);}
	fseek(fd, 512 * block + 16*n, SEEK_SET);
	// write(fd, &entry, 16);
	int error = fwrite(&entry, 16, 1, fd);
	if(debug){printf(" fwrite returned: %d\n",error );}
}




// Main function that contains the input loop
int main(int argc, char *argv[]){

	fd = -1;

	// if (argc > 1){
	// 	if (strcmp(argv[1], "-d") == 0){ debug=1; }else{ debug=0; }
	// }

	if (debug){ printf("|Running in debug mode.\n");}



	// Input execution loop:
	while(1){
		// get input into buffer and null terminate it
		char input[100]; // command input buffer
		int j;
		for(j=0; j < 100; j++){ input[j] = '\0'; }

		printf(">");
		fflush(stdin);
		int error = fgets(input, 100, stdin);
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
			if       (strcmp(args[0],"mount"     )==0){ mount     (args[1]);
			}else if (strcmp(args[0],"ckfiletype")==0){ ckFileType(args[1]);
			}else if (strcmp(args[0],"filesize"  )==0){ fileSize  (args[1]);
			}else if (strcmp(args[0],"initfs"    )==0){ initfs    (args[1], args[2]);
			}else if (strcmp(args[0],"cpin"      )==0){ cpin      (args[1], args[2]);
			}else if (strcmp(args[0],"cpout"     )==0){ cpout     (args[2], args[1]);
			}else if (strcmp(args[0],"mkdir"     )==0){ mkdirectory( args[1] );
			}else if (strcmp(args[0],"rm"        )==0){ removedir( args[1] );
			}else if (strcmp(args[0],"h"         )==0 ||
								strcmp(args[0],"help"      )==0){	printHelp();
			}else if (strcmp(args[0],"d"         )==0 ||
								strcmp(args[0],"debug"     )==0){ toggleDebug(args[1]);
			}else if (strcmp(args[0],"q"         )==0){
				printf(" Exiting...\n");
				if (fd != -1){ fflush(fd); fclose(fd); }
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
inode getInode( int n ){
	inode node;
	if (fd == -1){
		printf(" Error getting inode: No filesystem mounted.\n");
		return node;
	}
	// if (debug){ printf("|Getting inode...\n"); }
	// if (debug){ printf("|Seeking to %d\n", 512*2 + (n-1)*32); }
	fseek(fd, 512 * 2 + (n-1) * 32, SEEK_SET);
	// if (debug){ printf("|Reading from filesystem to node struct...\n"); }
	fread(&node, 1, sizeof(node), fd);
	// if (debug){ printf("|Got iNode successfully.\n"); }
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

int cpout( char * outsidePath, char * insidePath){
	if (fd == -1){
		printf(" Error copying file: no filesystem mounted.\n");
		return -1;
	}

	if (outsidePath == NULL || insidePath == NULL || strlen(outsidePath) < 1 || strlen(insidePath) < 1){
		printf(" Error copying file: malformed args.\n");
		return -1;
	}


	// Split the input path on "/"
	char delim[] = "/";
	int i = 0;
	char * args[50];
	args[i] = strtok(insidePath, delim);
	while(args[i] != NULL){args[++i]=strtok(NULL,delim);}


	if(debug){printf("|Attempting to find: %s\n", insidePath);}

	unsigned short currentAddress = 1;
	inode directory = getInode(1);

	// For each part of the path:
	for (i = 0; args[i] != NULL; i++){
		if(debug){printf("|Searching for path element: %s\n", args[i]);}
		// mkdir user,files,a,NULL,....
		if (args[i+1] == NULL){
			int j;
			for (j = 0; j < 8; j++){
				if (directory.addr[j] < 1){
					if(debug){printf("|Reached end of directory entries.\n");}
					break;
				}

				if(debug){printf("|Searching directory entries in block %d\n", directory.addr[j] );}

				int k;
				for (k = 0; k < 32; k++){
					directoryEntry file = readDirectoryEntry( directory.addr[j], k);
					if (debug) { printf("| Reading block %d, entry %d (%x) - ", directory.addr[j],k, 512 * directory.addr[j] + 16*k);}
					if (debug) { printf("Checking if \"%s\" == \"%s\"\n", args[i], file.name);}
					if (strcmp((char *)file.name, args[i]) == 0){


						inode node = getInode(file.inode);
						int l;
						int outsideFile = fopen(outsidePath, "w+b");
						int bytesToWrite = node.size1;
						for (l = 0; l < 8; l++){
							if (node.addr[l] > 0 && bytesToWrite > 0){
								charBlock block;
								fseek(fd, 512 * node.addr[l], SEEK_SET);
								int bytesRead = fread(&block, 512, 1, fd);
								if (bytesToWrite < 512){
									fwrite(&block, bytesToWrite, 1, outsideFile);
								}else{
									fwrite(&block, 512, 1, outsideFile);
									bytesToWrite -= bytesRead;
								}

							}
						}
						int error = fclose(outsideFile);
						return 0;
					}
				}
			}
			return 0;
		}else{

			int j;
			for (j = 0; j < 8; j++){
				if (directory.addr[j] > 1){
					int k;
					for (k = 0; k < 512/16; k++){
						directoryEntry file = readDirectoryEntry( directory.addr[j], k);

						if (strcmp(file.name, args[i]) == 0){

							inode newDirectory = getInode(file.inode);
							if (newDirectory.flags & 040000){
								directory = getInode(file.inode);
								currentAddress = file.inode;
								j = 8;
								break;
							}else{
								printf(" %s is not a directory.\n", args[i]);
							}
						}
					}
				}else{
					printf(" No such path in the V6 filesystem:\n ");
					int k;
					for(k = 0; k <=i; k++){
						printf("/%s", args[k]);
					}
					printf("\n");
					return -1;
				}
			}
		}
	}
	printf("\n");
	return 1;
}

//////////////////////////////////////////////////////////////////////////////

int cpin( char * outsidePath, char * insidePath){
	if (fd == -1){
		printf(" Error copying file: no filesystem mounted.\n");
		return -1;
	}

	if (outsidePath == NULL || insidePath == NULL || strlen(outsidePath) < 1 || strlen(insidePath) < 1){
		printf(" Error copying file: malformed args.\n");
		return -1;
	}


	// Split the input path on "/"
	char delim[] = "/";
	int i = 0;
	char * args[50];
	args[i] = strtok(insidePath, delim);
	while(args[i] != NULL){args[++i]=strtok(NULL,delim);}


	if(debug){printf("|Attempting to find: %s\n", insidePath);}

	unsigned short currentAddress = 1;
	inode directory = getInode(1);

	// For each part of the path:
	for (i = 0; args[i] != NULL; i++){
		if(debug){printf("|Searching for path element: %s\n", args[i]);}
		// mkdir user,files,a,NULL,....
		if (args[i+1] == NULL){
			if(debug){printf("|Searching for inode for file: %s\n", args[i]);}
			// Make an inode
			inode node;
			int l;
			if(debug){printf("|Searching through %d inodes in %d blocks.\n", super.isize*16, super.isize);}
			for (l = 1; l < super.isize * 16 + 1; l++){
				node = getInode(l);
				if (! (node.flags & 0100000) ){
					if(debug){printf("| Inode %d with flags: %o found.\n", l, node.flags );}

					if(debug){printf("|Searching if file already exists ...\n");}

					int j;
					for (j = 0; j < 8; j++){
						if (directory.addr[j] < 1){
							if(debug){printf("|Reached end of directory entries.\n");}
							break;
						}

						if(debug){printf("|Searching directory entries in block %d\n", directory.addr[j] );}

						int k;
						for (k = 0; k < 32; k++){
							directoryEntry file = readDirectoryEntry( directory.addr[j], k);
							if (debug) { printf("| Reading block %d, entry %d (%x) - ", directory.addr[j],k, 512 * directory.addr[j] + 16*k);}
							if (debug) { printf("Checking if \"%s\" == \"%s\"\n", args[i], file.name);}
							if (strcmp((char *)file.name, args[i]) == 0){
								printf(" Error making file: file %s already exists.\n", args[i] );
								return 0;
							}
						}
					}
					if(debug){printf("|Finding unallocated directory entry...\n");}

					for (j = 0; j < 8; j++){
						if (directory.addr[j] < 1){
							if(debug){printf("|Directory data block full, adding new block...\n");}
							directory.addr[j] = allocateBlock();
						}


						int k;
						for (k = 0; k < 32; k++){

							directoryEntry file = readDirectoryEntry( directory.addr[j], k );

							if(debug){printf("|Checking file #%d, inode:%d, \"%s\"\n", k, file.inode, file.name);}
							if (file.inode == 0){
								if(debug){printf("|Found unallocated directory entry #%d.\n", k);}

								int outsideFile = fopen(outsidePath, "rb");
								fseek(outsideFile, 0, SEEK_END);
								int size = ftell(outsideFile);
								fseek(outsideFile, 0, SEEK_SET);
								if(debug){printf("|File to be copied is of size: %d.\n", size);}

								node.flags = 0100000;
								node.size0 = 0;
								node.size1 = size;

								int adderIndex = 0;

								while(size > 0){
									if (adderIndex == 8){
										printf("File too large (>4kB). Truncating rest of file.\n");
										break;
									}


									node.addr[adderIndex] = allocateBlock();
									if(debug){printf("|Allocating block #%d.\n", node.addr[adderIndex]);}
									int readSize = 512;

									charBlock block;

									if (size < 512){
										if (debug){printf("|Size < 512, changing readsize\n");}
										int m;
										for (m = 0; m < 512; m++){ block.contents[m] = '\0'; }
										readSize = size;
									}

									if(debug){printf("|Reading %d bytes from outside file: %d\n", readSize, outsideFile);}

									int bytesRead = fread(&block, 1, readSize, outsideFile);

									// if(debug){printf("|Read = %d\n", error);}
									if(debug){printf("|Read \"%s\" from outside file.\n", block.contents);}
									if(debug){printf("|Read block contins %d bytes.\n", sizeof(block.contents));}
									if(debug){printf("|writing to inside file...\n");}


									writeCharBlock(node.addr[adderIndex], block);

									if(debug){printf("|writing successful.\n");}
									size -= 512;
									adderIndex++;
								}
								fclose(outsideFile);

								if(debug){printf("|Writting new directory's inode.\n");}
								writeInode( l, node );

								if(debug){printf("|Writting parent directory's entry for new directory...\n");}

								directoryEntry newFile = { l, "" };
								strcpy((char *) newFile.name, args[i]);

								if(debug){printf("|Writting new file entry \"%s\", inode:%d\n",newFile.name, newFile.inode );}
								writeDirectoryEntry( directory.addr[j], k, newFile);

								return 1;
							}
						}
					}
					printf(" Error: Directory full.\n");
					return -1;

				}else{
					if(debug){printf("| Inode %d with flags: %o not available.\n", l, node.flags );}
				}
			}
			printf(" Error making directory: directory full / no inode available.\n" );
			return 0;
		}else{

			int j;
			for (j = 0; j < 8; j++){
				if (directory.addr[j] > 1){
					int k;
					for (k = 0; k < 512/16; k++){
						directoryEntry file = readDirectoryEntry( directory.addr[j], k);

						if (strcmp(file.name, args[i]) == 0){

							inode newDirectory = getInode(file.inode);
							if (newDirectory.flags & 040000){
								directory = getInode(file.inode);
								currentAddress = file.inode;
								j = 8;
								break;
							}else{
								printf(" %s is not a directory.\n", args[i]);
							}
						}
					}
				}else{
					printf(" No such path in the V6 filesystem:\n ");
					int k;
					for(k = 0; k <=i; k++){
						printf("/%s", args[k]);
					}
					printf("\n");
					return -1;
				}
			}
		}
	}
	printf("\n");
	return 1;
}

///////////////////////////////////////////////////////////////////////////////

int removedir(char * name){
	if (fd == -1){
		printf(" Error remove directory: no filesystem mounted.\n");
		return -1;
	}
	if (strlen(name) < 1){
		printf(" Error removing directory: no name given.\n");
		return -1;
	}

	// Split the input path on "/"
	char delim[] = "/";
	int i = 0;
	char * args[50];
	args[i] = strtok(name, delim);
	while(args[i] != NULL){args[++i]=strtok(NULL,delim);}


	if(debug){printf("|Attempting to find: %s\n", name);}

	unsigned short currentAddress = 1;
	inode directory = getInode(1);

	for (i = 0; args[i] != NULL; i++){
		if(debug){printf("|Searching for path element: %s\n", args[i]);}
		// mkdir user,files,a,NULL,....
		if (args[i+1] == NULL){
			if(debug){printf("|Searching for inode for file: %s\n", args[i]);}

			int j;
			for (j = 0; j < 8; j++){
				if (directory.addr[j] < 1){
					if(debug){printf("|Reached end of directory entries.\n");}
					break;
				}

				if(debug){printf("|Searching directory entries in block %d\n", directory.addr[j] );}

				int k;
				for (k = 0; k < 32; k++){
					directoryEntry file = readDirectoryEntry( directory.addr[j], k);
					if (debug) { printf("| Reading block %d, entry %d (%x) - ", directory.addr[j],k, 512 * directory.addr[j] + 16*k);}
					if (debug) { printf("Checking if \"%s\" == \"%s\"\n", args[i], file.name);}
					if (strcmp((char *)file.name, args[i]) == 0){
						if (debug){ printf("|File found in entry %d.\n", k );}
						inode node = getInode(file.inode);

						if (!node.flags & 100000){
							printf(" Error: file's inode does not exist.\n");
							return -1;
						}
						int l;
						if (node.flags & 040000){

							for (l = 0; l < 8; l++){
								if (node.addr[l] > 1){
									int m;
									for (m = 0; m < 32; m++){
											directoryEntry entry = readDirectoryEntry(node.addr[l], m);
											if ( !( (l == 0) && (m<2) ) && (entry.inode != 0)){
												printf(" Error removing file: file is directory that contains files.\n");
												return -1;
											}
									}
								}
							}
						}

						directoryEntry blankFile = {0, ""};
						writeDirectoryEntry( directory.addr[j], k, blankFile);

						for (l = 0; l < 8; l++){
							if (node.addr[l] > 1){
								if(debug){printf("|Freeing node address[%d]: %d (%x)\n",l, node.addr[l] ,node.addr[l]*512 );}
								freeBlock( node.addr[l] );
							}
						}

						node.flags = 0;
						writeInode(file.inode, node);

						return 0;
					}
				}
			}
			printf(" Error: File does not exist in directory.\n");
			return -1;
		}else{

			int j;
			for (j = 0; j < 8; j++){
				if (directory.addr[j] > 1){
					int k;
					for (k = 0; k < 512/16; k++){
						directoryEntry file = readDirectoryEntry( directory.addr[j], k);

						if (strcmp(file.name, args[i]) == 0){

							inode newDirectory = getInode(file.inode);
							if (newDirectory.flags & 040000){
								directory = getInode(file.inode);
								currentAddress = file.inode;
								j = 8;
								break;
							}else{
								printf(" %s is not a directory.\n", args[i]);
							}
						}
					}
				}else{
					printf(" No such path in the V6 filesystem:\n ");
					int k;
					for(k = 0; k <=i; k++){
						printf("/%s", args[k]);
					}
					printf("\n");
					return -1;
				}
			}
		}


	}
	printf("\n");

}

///////////////////////////////////////////////////////////////////////////////
int mkdirectory(char * name){
	if (fd == -1){
		printf(" Error making directory: no filesystem mounted.\n");
		return -1;
	}
	if (strlen(name) < 1){
		printf(" Error making directory: no name given.\n");
		return -1;
	}

	// Split the input path on "/"
	char delim[] = "/";
	int i = 0;
	char * args[50];
	args[i] = strtok(name, delim);
	while(args[i] != NULL){args[++i]=strtok(NULL,delim);}


	if(debug){printf("|Attempting to find: %s\n", name);}

	unsigned short currentAddress = 1;
	inode directory = getInode(1);

	// For each part of the path:
	for (i = 0; args[i] != NULL; i++){
		if(debug){printf("|Searching for path element: %s\n", args[i]);}
		// mkdir user,files,a,NULL,....
		if (args[i+1] == NULL){
			if(debug){printf("|Searching for inode for file: %s\n", args[i]);}
			// Make an inode
			inode node;
			int l;
			if(debug){printf("|Searching through %d inodes in %d blocks.\n", super.isize*16, super.isize);}
			for (l = 1; l < super.isize * 16 + 1; l++){
				node = getInode(l);
				if (! (node.flags & 0100000) ){
					if(debug){printf("| Inode %d with flags: %o found.\n", l, node.flags );}

					if(debug){printf("|Searching if file already exists ...\n");}

					int j;
					for (j = 0; j < 8; j++){
						if (directory.addr[j] < 1){
							if(debug){printf("|Reached end of directory entries.\n");}
							break;
						}

						if(debug){printf("|Searching directory entries in block %d\n", directory.addr[j] );}

						int k;
						for (k = 0; k < 32; k++){
							directoryEntry file = readDirectoryEntry( directory.addr[j], k);
							if (debug) { printf("| Reading block %d, entry %d (%x) - ", directory.addr[j],k, 512 * directory.addr[j] + 16*k);}
							if (debug) { printf("Checking if \"%s\" == \"%s\"\n", args[i], file.name);}
							if (strcmp((char *)file.name, args[i]) == 0){
								printf(" Error making directory: file %s already exists.\n", args[i] );
								return 0;
							}
						}
					}
					if(debug){printf("|Finding unallocated directory entry...\n");}

					for (j = 0; j < 8; j++){
						if (directory.addr[j] < 1){
							if(debug){printf("|Directory data block full, adding new block...\n");}
							directory.addr[j] = allocateBlock();
						}


						int k;
						for (k = 0; k < 32; k++){

							directoryEntry file = readDirectoryEntry( directory.addr[j], k );

							if(debug){printf("|Checking file #%d, inode:%d, \"%s\"\n", k, file.inode, file.name);}
							if (file.inode == 0){
								if(debug){printf("|Found unallocated directory entry #%d.\n", k);}

								node.flags = 0140000;
								node.size0 = 0;
								node.size1 = 32;
								node.addr[0] = allocateBlock();

								directoryEntry autoEntries[2] = {
									{l,              "." },
									{currentAddress, ".."}
								};

								if(debug){printf("|Writting new directory's contents (., ..)\n");}
								writeDirectoryEntry( node.addr[0], 0, autoEntries[0]);
								writeDirectoryEntry( node.addr[0], 1, autoEntries[1]);


								if(debug){printf("|Writting new directory's inode.\n");}
								writeInode( l, node );

								if(debug){printf("|Writting parent directory's entry for new directory...\n");}

								directoryEntry newFile = { l, "" };
								strcpy((char *) newFile.name, args[i]);

								if(debug){printf("|Writting new file entry \"%s\", inode:%d\n",newFile.name, newFile.inode );}

								writeDirectoryEntry( directory.addr[j], k, newFile);

								return 1;
							}
						}
					}
					printf(" Error: Directory full.\n");
					return -1;

				}else{
					if(debug){printf("| Inode %d with flags: %o not available.\n", l, node.flags );}
				}
			}
			printf(" Error making directory: directory full / no inode available.\n" );
			return 0;
		}else{

			int j;
			for (j = 0; j < 8; j++){
				if (directory.addr[j] > 1){
					int k;
					for (k = 0; k < 512/16; k++){
						directoryEntry file = readDirectoryEntry( directory.addr[j], k);

						if (strcmp(file.name, args[i]) == 0){

							inode newDirectory = getInode(file.inode);
							if (newDirectory.flags & 040000){
								directory = getInode(file.inode);
								currentAddress = file.inode;
								j = 8;
								break;
							}else{
								printf(" %s is not a directory.\n", args[i]);
							}
						}
					}
				}else{
					printf(" No such path in the V6 filesystem:\n ");
					int k;
					for(k = 0; k <=i; k++){
						printf("/%s", args[k]);
					}
					printf("\n");
					return -1;
				}
			}
		}


	}
	printf("\n");
}


//////////////////////////////////////////////////////////////////////////////
int initfs( char * n1, char * n2){

	if (fd == -1){
		printf(" Error initializing filesystem: no filesystem mounted.\n");
		return -1;
	}
	if (n1 == NULL && n2 == NULL){
		printf(" Error initializing filesystem: arguments invalid.\n");
		return -1;
	}

	fflush(fd);
	fclose(fd);
	fd = fopen( filesystemName, "w+b");

	if (fd == -1){
		printf("Could not clear file \"%s\", also mounted file is closed sorry\n", filesystemName);
		return -1;
	}


	unsigned short numBlocks;
	unsigned short iBlocks;
	if (n1){ numBlocks = atoi(n1); }
	if (n2){ iBlocks   = atoi(n2); }
	if (iBlocks >= numBlocks){
		printf(" Error initializing filesystem: too many iNode blocks.\n");
		return -1;
	}

	super.isize  = iBlocks;
	super.fsize  = numBlocks;
	super.nfree  = 0;
	super.ninode = 100;
	super.time[0] = 2;
	super.time[1] = 2;


	int i;
	for (i = 0; i < 100; i++){
			super.free[i] = 0;
	}

	writeSuperblock();

	for (i = iBlocks + 2; i < numBlocks; i++){
		freeBlock( (unsigned short) i);
	}

	inode node;
	node.flags = 0100000 | 040000;
	node.nlinks = 1;
	node.uid = '\0';
	node.gid = '\0';
	node.size0 = '\0';
	node.size1 = 32;

	node.addr[0] = allocateBlock();
	node.addr[1] = 0;
	node.addr[2] = 0;
	node.addr[3] = 0;
	node.addr[4] = 0;
	node.addr[5] = 0;
	node.addr[6] = 0;
	node.addr[7] = 0;

	if(debug){printf("|Allocated block #%d to the root directory.\n", node.addr[0]);}

	directoryEntry autoEntries = {1,"ROOT"};

	if(debug){printf("|Writting root directory's contents (.)\n");}
	writeDirectoryEntry( node.addr[0], 0, autoEntries);


	if(debug){printf("|Writting root directory's inode.\n");}
	writeInode(1, node);
	// writeCharBlock( 2, (char *) &node);
	fflush(fd);
	fclose(fd);
	fd = fopen( filesystemName, "r+b");
}
/////////////////////////////////////////////////////////////////////////////
int allocateBlock(){
	if (fd == -1){
		printf(" Error allocating block: no filesystem mounted.\n");
		return -1;
	}

	if(debug){ printf("|Allocating block...\n");}
	if(debug){ printf("| super.nfree = %d\n", super.nfree);}
	if (super.nfree > 0){

		int blockNumber = super.free[super.nfree - 1];
		if(debug){ printf("| blockNumber = %d\n", blockNumber);}
		super.nfree--;
		if (super.nfree == 0){
			if(debug){ printf("| Reading chainblock at %d (%x)\n", super.free[0], super.free[0] * 512);}
			chainBlock block = readChainBlock(super.free[0]);
			if(debug){ printf("| Read Chainblock.\n");}
			// block = readChainBlock(block.addresses[0]);

			if(debug){ printf("| Setting super.nfree = %d\n", block.size);}
			super.nfree = block.size;
			if(debug){ printf("|  setting addresses...\n");}
			int i;
			for (i = 0; i < super.nfree; i++){
				super.free[i] = block.addresses[i];
			}
			if(debug){ printf("|  addresses set.\n");}

		}
		if(debug){ printf("| Updating superblock...\n");}
		writeSuperblock();
		if(debug){ printf("| Done.\n");}

		charBlock clear;
		int i;
		for (i=0;i<512;i++){clear.contents[i]='\0';}
		writeCharBlock(blockNumber, clear);

		return blockNumber;
	}else{
		printf(" Error allocating block: filesystem full.\n");
		return -1;
	}
}
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
int freeBlock (unsigned short n){
	if (fd == -1){
		printf(" Error freeing block: no filesystem mounted.\n");
		return -1;
	}
	if ( n <= 1 ){
		printf(" Error freeing block: block reserved.\n");
		return -1;
	}


	if (super.nfree < 100){
		super.free[super.nfree] = n;
		super.nfree++;

		charBlock block;
		int i;
		for (i=0; i < 512; i++){
			block.contents[i] = '\0';
		}
		writeCharBlock(n, block);
		writeSuperblock();
		return 1;
	}else{

		chainBlock block;

		block.size = super.nfree;

		int i;
		for (i = 0; i < super.nfree; i++){
			block.addresses[i] = super.free[i];
		}
		charBlock clear;
		for (i=0;i<512;i++){clear.contents[i]='\0';}
		writeCharBlock(n, clear);
		writeChainBlock(n, block);
		super.nfree = 0;
		super.free[super.nfree] = n;
		super.nfree++;
		writeSuperblock();
		return 1;

	}
}
//////////////////////////////////////////////////////////////////////////////



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

	fd = fopen(path, "r+b");

	strcpy( filesystemName, path);

	readSuperblock();
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
int ckFileType( char * in ){
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

	inode node = (inode) getInode( n );
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

	fileSize( in );

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// filesize
// Check inode and display its file's size
int fileSize( char * in ){

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

	inode node = (inode) getInode( m );
	if (debug){ printf("| Size 0 = %d\n", (int)(node.size0)); }
	if (debug){ printf("| Size 1 = %d\n", (int)(node.size1)); }

	int size0 = ((int)(node.size0)) << 16;
	int size1 = (int)(node.size1);
	int size = size0 + size1;

	printf(" Size of file: %d bytes\n",size);
}
