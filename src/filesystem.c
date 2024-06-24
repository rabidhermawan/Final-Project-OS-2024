#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void fsInit() {
  struct map_fs map_fs_buf;
  int i = 0;

  readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
  for (i = 0; i < 16; i++) map_fs_buf.is_used[i] = true;
  for (i = 256; i < 512; i++) map_fs_buf.is_used[i] = true;
  writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
}

// TODO: 2. Implement fsRead function
void fsRead(struct file_metadata* metadata, enum fs_return* status) {
  struct node_fs node_fs_buf;
  struct data_fs data_fs_buf;

  int i, nodeIndex;
  nodeIndex = -1;

  readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  for (i = 0; i < FS_MAX_NODE; ++i) {
    if (
      strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name) && 
      node_fs_buf.nodes[i].parent_index == metadata->parent_index)
      {
        nodeIndex = i;
        break;
    }
  }

  if (nodeIndex == -1) {
    *status = FS_R_NODE_NOT_FOUND;
    return;
  }
  
  if (node_fs_buf.nodes[nodeIndex].data_index == FS_NODE_D_DIR){
    *status = FS_R_TYPE_IS_DIRECTORY;
    return;
  }

  //Prepare to read file
  metadata->filesize = 0;
  for (i = 0; i < FS_MAX_SECTOR; i++) {
    if (data_fs_buf.datas[node_fs_buf.nodes[nodeIndex].data_index].sectors[i] == 0x00) {
      break;
    }
    readSector(metadata->buffer + i * SECTOR_SIZE, data_fs_buf.datas[node_fs_buf.nodes[nodeIndex].data_index].sectors[i]);
    metadata->filesize += SECTOR_SIZE;
  }

  *status = FS_SUCCESS;
}

// TODO: 3. Implement fsWrite function
void fsWrite(struct file_metadata* metadata, enum fs_return* status) {
  //---Initialization---
  struct map_fs map_fs_buf;
  struct node_fs node_fs_buf;
  struct data_fs data_fs_buf;

  int i, j, dataIndex, sectorIndex, nodeIndex, mapIndex, emptyCounter;
  nodeIndex = dataIndex = mapIndex = -1;
  j = emptyCounter = 0;
  readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);
  readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  //---Finding existing node---
  for (i = 0; i < FS_MAX_NODE; ++i) {
    if (
        strcmp(metadata->node_name, node_fs_buf.nodes[i].node_name) &&
        (metadata->parent_index == node_fs_buf.nodes[i].parent_index)) 
      {
      *status = FS_W_NODE_ALREADY_EXISTS;
      return;
    }
    if (node_fs_buf.nodes[i].node_name[0] == 0x00 && nodeIndex == -1){
      nodeIndex = i;
      break;
    }
  }

  if (nodeIndex == -1){
    *status = FS_W_NO_FREE_NODE;
    return;
  }

  //---If the file is a directory---
  if (metadata->filesize == 0){
    strcpy(node_fs_buf.nodes[nodeIndex].node_name, metadata->node_name);
    node_fs_buf.nodes[nodeIndex].parent_index = metadata->parent_index;
    node_fs_buf.nodes[nodeIndex].data_index = FS_NODE_D_DIR;
    
    writeSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);
    writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
    *status = FS_SUCCESS;
    return;
  }
  
  //---Find empty data---
  for (i = 0; i < FS_MAX_DATA; ++i) {
    if (data_fs_buf.datas[i].sectors[0] == 0x00){
        dataIndex = i;
        break;
    }
  }
  if (dataIndex == -1){
    *status = FS_W_NO_FREE_DATA;
    return;
  }

  //---Count empty map---
  for (i = 0; i < SECTOR_SIZE; ++i)
    if (!map_fs_buf.is_used[i]) emptyCounter++;

  if (emptyCounter < (metadata->filesize / SECTOR_SIZE) + ((metadata->filesize % SECTOR_SIZE) ? 1 : 0)){
    *status = FS_W_NOT_ENOUGH_SPACE;
    return;
  }

  //---Write to new node---
  strcpy(node_fs_buf.nodes[nodeIndex].node_name, metadata->node_name);
  node_fs_buf.nodes[nodeIndex].parent_index = metadata->parent_index;
  node_fs_buf.nodes[nodeIndex].data_index = dataIndex;
  
  for (i = 0; i < SECTOR_SIZE; ++i) {
    if (map_fs_buf.is_used[i] == 0x00) {
      data_fs_buf.datas[dataIndex].sectors[j] = i;
      writeSector(metadata->buffer + j * SECTOR_SIZE, i);
      map_fs_buf.is_used[i] == true;
      j++;
    }
  }
  
  writeSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);
  writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
  writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
  *status = FS_SUCCESS;
  return;
}

void fsStatus(enum fs_return* status) {
  if (*status == FS_UNKOWN_ERROR)
    printString("FS_UNKOWN_ERROR\n");

  if (*status == FS_SUCCESS)
    printString("FS_SUCCESS\n");
  
  //Read return code
  if (*status == FS_R_NODE_NOT_FOUND)
    printString("FS_R_NODE_NOT_FOUND\n");

  if (*status == FS_R_TYPE_IS_DIRECTORY)
    printString("FS_R_TYPE_IS_DIRECTORY\n");

  //Write return code
  if (*status == FS_W_NODE_ALREADY_EXISTS)
    printString("FS_W_NODE_ALREADY_EXISTS\n");

  if (*status == FS_W_NOT_ENOUGH_SPACE)
    printString("FS_W_NOT_ENOUGH_SPACE\n");

  if (*status == FS_W_NO_FREE_NODE)
    printString("FS_W_NO_FREE_NODE\n");

  if (*status == FS_W_NO_FREE_DATA)
    printString("FS_W_NO_FREE_DATA\n");

  if (*status == FS_W_INVALID_DIRECTORY)
    printString("FS_W_INVALID_DIRECTORY\n");
}