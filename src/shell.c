#include "shell.h"
#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void shell() {
  char buf[64];
  char cmd[64];
  char arg[2][64];

  byte cwd = FS_NODE_P_ROOT;

  while (true) {
    printString("MengOS:");
    printCWD(cwd);
    printString("$ ");
    readString(buf);
    parseCommand(buf, cmd, arg);

    if (strcmp(cmd, "cd")) cd(&cwd, arg[0]);
    else if (strcmp(cmd, "ls")) ls(cwd, arg[0]);
    else if (strcmp(cmd, "mv")) mv(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cp")) cp(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cat")) cat(cwd, arg[0]);
    else if (strcmp(cmd, "mkdir")) mkdir(cwd, arg[0]);
    else if (strcmp(cmd, "clear")) clearScreen();
    else printString("Invalid command\n");
  }
}

// TODO: 4. Implement printCWD function
void printCWD(byte cwd) {
  struct node_fs node_fs_buf;
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  //---If root---
  if (cwd == FS_NODE_P_ROOT){
    printString("/");
    return;
  }

  //---Find the root---
  else {
    printCWD(node_fs_buf.nodes[cwd].parent_index);
    printString(node_fs_buf.nodes[cwd].node_name);
    printString("/");
  }
}

// TODO: 5. Implement parseCommand function
void parseCommand(char* buf, char* cmd, char arg[2][64]) {
  int i, j = 0, k = 0;
  
  for (i = 0; i < 64; ++i) {
	  cmd[i] = arg[0][i] = arg[1][0] = 0;
  }

  for (i = 0; i < strlen(buf); ++i) {
	  if (buf[i] == ' ') {
		  if (!j) 
			  cmd[k] = '\0';
		  if (j == 1) 
			  arg[0][k] = '\0';
		  if (j == 2) 
			  arg[1][k] = '\0';
		
      i++;
		  j++;
		  k = 0;
	  }
	  if (!j) 
		  cmd[k++] = buf[i];
	  if (j == 1) 
		  arg[0][k++] = buf[i];
	  if (j == 2)
		  arg[1][k++] = buf[i];
  }
  arg[1][k] = '\0';
}

// TODO: 6. Implement cd function
void cd(byte* cwd, char* dirname) {
  struct node_fs node_fs_buf;
  int i;
  char testing;

  if (!dirname[0]){
    printString("cd: missing operand\n");
    return;
  }

  if (strcmp(dirname, "/")){
    *cwd = FS_NODE_P_ROOT;
    return;
  }

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
  
  if (strcmp(dirname, "..")){
    if (*cwd == FS_NODE_P_ROOT) return;
    *cwd = node_fs_buf.nodes[*cwd].parent_index;
    return;
  }
  
  for (i = 0; i < FS_MAX_NODE; ++i) {
    if (strcmp(node_fs_buf.nodes[i].node_name, dirname) && node_fs_buf.nodes[i].parent_index == *cwd) {
      if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
          *cwd = i;
      return;
      }
      else {
        printString("cd: ");
        printString(dirname);
        printString(": Not a directory\n");
        return;
      }
    }
  }

  printString("cd: ");
  printString(dirname);
  printString(": No such file or directory\n");
}

// TODO: 7. Implement ls function
void ls(byte cwd, char* dirname) {
  struct node_fs node_fs_buf;
  int i, targetCwd;
  bool aFile;
  aFile = false;
  targetCwd = -1;
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
  if (!strlen(dirname) || strcmp(dirname, ".")) targetCwd = cwd;
  else {
    for (i = 0; i < FS_MAX_NODE; ++i) {
      if (node_fs_buf.nodes[i].parent_index == cwd && strcmp(node_fs_buf.nodes[i].node_name, dirname)) {
        targetCwd = i;
        break;
      }
    }

    if (targetCwd == -1) {
      printString("ls: Directory doesn't exist\n");
      return;
    }
  }

  for (i = 0; i < FS_MAX_NODE; ++i) {
      if (node_fs_buf.nodes[i].parent_index == cwd && node_fs_buf.nodes[i].node_name[0] != 0) {
        aFile = true;
        printString(node_fs_buf.nodes[i].node_name);
        printString(" ");
      }
  }
  if (aFile) printString("\n");
}

// TODO: 8. Implement mv function
void mv(byte cwd, char* src, char* dst) {
  struct node_fs node_fs_buf;
  char dstSplit[2][64];
  int i, parentIndex, atCurrentDir, targetIndex;
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  targetIndex = -1;
  //Find the file
  for (i = 0; i < FS_MAX_NODE; ++i) {
    if (strcmp(node_fs_buf.nodes[i].node_name, src) && node_fs_buf.nodes[i].parent_index == cwd) {
      if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR){
        printString("mv: ");
        printString(src);
        printString(" is a directory\n");
        return;
      }
      targetIndex = i;
    }
  }

  if (targetIndex == -1) {
    printString("mv: file not found\n");
    return;
  }

  //Now let's move
  clear(dstSplit[0], 64);
  clear(dstSplit[1], 64);
  splitSentence(dst, dstSplit[0], dstSplit[1], '/');

  parentIndex = -1;
  atCurrentDir = 0;
  //Determine parent index for later
  //If previous destination
  if (strcmp(dstSplit[0], "..\0")) {
    parentIndex = node_fs_buf.nodes[cwd].parent_index;
  }
  //If parent index
  else if (!strlen(dstSplit[0])) {
    parentIndex = FS_NODE_P_ROOT;
  }
  //If at current directory
  else if (strlen(dstSplit[0]) && !strlen(dstSplit[1])){
    parentIndex = cwd;
    atCurrentDir = 1;
  }
  //If at current destination
  else {
    for (i = 0; i < FS_MAX_NODE; ++i) {
      if (strcmp(node_fs_buf.nodes[i].node_name, dstSplit[0]) && node_fs_buf.nodes[i].parent_index == cwd) {
        parentIndex = i;
      }
    }

    if (parentIndex == -1) {
      printString("mv: directory not found\n");
      return;
    }
  }
  
  //Check if the filename exist
  for (i = 0; i < FS_MAX_NODE; ++i) {
    if (!atCurrentDir) {
      if (strcmp(node_fs_buf.nodes[i].node_name, dstSplit[1]) && node_fs_buf.nodes[i].parent_index == parentIndex) {
        printString("mv: filename exists\n");
        return;
      }
    }
    else {
      if (strcmp(node_fs_buf.nodes[i].node_name, dstSplit[0]) && node_fs_buf.nodes[i].parent_index == parentIndex) {
        printString("mv: filename exists\n");
        return;
      }
    }
  }

  //Change the properties
  node_fs_buf.nodes[targetIndex].parent_index = parentIndex;
  if (!atCurrentDir) strcpy(node_fs_buf.nodes[targetIndex].node_name, dstSplit[1]);
  else strcpy(node_fs_buf.nodes[targetIndex].node_name, dstSplit[0]);

  //Write the damn nodes!
  writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
}

// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst) {
  struct file_metadata targetFile;
  enum fs_return fsReturn;
  char dstSplit[2][64];
  int i, parentIndex, atCurrentDir, targetIndex;
  struct node_fs node_fs_buf;
  
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  targetFile.parent_index = cwd;
  targetFile.filesize = 0;
  strcpy(targetFile.node_name, src);
  fsRead(&targetFile, &fsReturn);
  
  if (fsReturn == FS_R_NODE_NOT_FOUND){
    printString("cp: ");
    printString(src);
    printString(": No such file\n");
    return;
  }
  if (fsReturn == FS_R_TYPE_IS_DIRECTORY){
    printString("cp: ");
    printString(src);
    printString(": Is a directory\n");
    return;
  }

  //Now let's copy
  clear(dstSplit[0], 64);
  clear(dstSplit[1], 64);
  splitSentence(dst, dstSplit[0], dstSplit[1], '/');

  parentIndex = -1;
  atCurrentDir = 0;
  //Determine parent index for later
  //If previous destination
  if (strcmp(dstSplit[0], "..\0")) {
    parentIndex = node_fs_buf.nodes[cwd].parent_index;
  }
  //If parent index
  else if (!strlen(dstSplit[0])) {
    parentIndex = FS_NODE_P_ROOT;
  }
  //If at current directory
  else if (strlen(dstSplit[0]) && !strlen(dstSplit[1])){
    parentIndex = cwd;
    atCurrentDir = 1;
  }
  //If at current destination
  else {
    for (i = 0; i < FS_MAX_NODE; ++i) {
      if (strcmp(node_fs_buf.nodes[i].node_name, dstSplit[0]) && node_fs_buf.nodes[i].parent_index == cwd) {
        parentIndex = i;
      }
    }

    if (parentIndex == -1) {
      printString("mv: directory not found\n");
      return;
    }
  }
  
  //Check if the filename exist
  for (i = 0; i < FS_MAX_NODE; ++i) {
    if (!atCurrentDir) {
      if (strcmp(node_fs_buf.nodes[i].node_name, dstSplit[1]) && node_fs_buf.nodes[i].parent_index == parentIndex) {
        printString("cp: filename exists\n");
        return;
      }
    }
    else {
      if (strcmp(node_fs_buf.nodes[i].node_name, dstSplit[0]) && node_fs_buf.nodes[i].parent_index == parentIndex) {
        printString("cp: filename exists\n");
        return;
      }
    }
  }

  targetFile.parent_index = parentIndex;
  
  if (!atCurrentDir) strcpy(targetFile.node_name, dstSplit[1]);
  else strcpy(targetFile.node_name, dstSplit[0]);
  fsWrite(&targetFile, &fsReturn);
  if (fsReturn == FS_W_NODE_ALREADY_EXISTS){
    printString("cp: ");
    printString(src);
    printString(": File already exits\n");
    return;
  }
  if (fsReturn == FS_W_NOT_ENOUGH_SPACE){
    printString("cp: ");
    printString(src);
    printString(": Not enough space\n");
    return;
  }
  if (fsReturn == FS_W_NO_FREE_NODE){
    printString("cp: ");
    printString(src);
    printString(": No free node\n");
    return;
  }
}

// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {
  struct file_metadata targetFile;
  enum fs_return fsReturn;

  targetFile.parent_index = cwd;
  targetFile.filesize = 0;
  strcpy(targetFile.node_name, filename);

  fsRead(&targetFile, &fsReturn);

  if (fsReturn == FS_R_NODE_NOT_FOUND){
    printString("cat: ");
    printString(filename);
    printString(": No such file or directory\n");
    return;
  }

  else if (fsReturn == FS_R_TYPE_IS_DIRECTORY){
    printString("cat: ");
    printString(filename);
    printString(": Is a directory\n");
    return;
  }

  printString("\n");
  printString(targetFile.buffer);
  printString("\n");
}

// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {
  struct file_metadata newFolder;
  enum fs_return fsReturn;

  if (!dirname[0]){
    printString("mkdir: missing operand\n");
    return;
  }

  newFolder.parent_index = cwd;
  newFolder.filesize = 0;
  strcpy(newFolder.node_name, dirname);
  clear(newFolder.buffer, 1);

  fsWrite(&newFolder, &fsReturn);

  if (fsReturn == FS_W_NO_FREE_NODE){
    printString("mkdir: cannot create directory '");
    printString(dirname);
    printString("': Not enough node\n");
  }
  if (fsReturn == FS_W_NODE_ALREADY_EXISTS){
    printString("mkdir: cannot create directory '");
    printString(dirname);
    printString("': File exists\n");
  }
}