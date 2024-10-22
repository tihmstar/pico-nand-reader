//
//  ChipProtocolNand8.cpp
//  pico-nand-reader
//
//  Created by tihmstar on 07.10.24.
//

#include "../include/ChipProtocolNand8.hpp"

#include "../include/macros.h"

#include <hardware/timer.h>
#include <hardware/clocks.h>
#include <tusb.h>

#include <stdio.h>

#include "proto_nand8.pio.h"

#define PROTO_NAND8_REQ_PIO       pio0
#define PROTO_NAND8_REQ_SM        0

#define PROTO_NAND8_RSP_PIO       pio1
#define PROTO_NAND8_RSP_SM        0

/*
  0-7     Data

  16      RB

  17      CLE
  18      ALE
  19      ~WE
  20      ~RE
  --------
  21      ~WP //0 protected //1 not protected
  22      CE0
  26      CE1
  27      CE2
  28      CE3
*/

#define NAND_RB     16

#define NAND_CLE    17
#define NAND_ALE    18
#define NAND_WE     19
#define NAND_RE     20

#define NAND_WP     21
#define NAND_CE0    22
#define NAND_CE1    26
#define NAND_CE2    27
#define NAND_CE3    28

#pragma mark ChipProtocolNand8
ChipProtocolNand8::ChipProtocolNand8()
: ChipProtocol(kChipProtocolNAND8)
,_proto_req_pio_pc{0}, _proto_rsp_pio_pc{0}
,_bufPageRead{NULL}, _bufPageIndex(0)
,_pageAddress(0), _readPages(0), _pageSize(0), _readCE(-1)
{
  for (int i=0; i<29; i++){
    gpio_init(i);
    gpio_set_dir(i, GPIO_OUT);
  }

  for (int i=0; i<16; i++){
    gpio_put(i, 0);
  }

  gpio_pull_up(NAND_RB);
  gpio_put(NAND_RE, 1);
  gpio_put(NAND_WE, 1);
  gpio_put(NAND_ALE, 0);
  gpio_put(NAND_CLE, 0);
  gpio_put(NAND_WP, 1);

  gpio_put(NAND_CE0, 1);
  gpio_put(NAND_CE1, 1);
  gpio_put(NAND_CE2, 1);
  gpio_put(NAND_CE3, 1);

  for (int i=0; i<17; i++){
    gpio_set_function(i, GPIO_FUNC_PIO0);
  }
  gpio_set_function(NAND_CLE, GPIO_FUNC_PIO0);
  gpio_set_function(NAND_ALE, GPIO_FUNC_PIO0);
  gpio_set_function(NAND_WE, GPIO_FUNC_PIO0);

  gpio_set_function(NAND_RE, GPIO_FUNC_PIO1);

  gpio_put(NAND_WP, 0); //write protected

  _proto_req_pio_pc = pio_add_program(PROTO_NAND8_REQ_PIO, &proto_nand8_req_program);
  proto_nand8_req_program_init(PROTO_NAND8_REQ_PIO, PROTO_NAND8_REQ_SM, _proto_req_pio_pc);
  pio_sm_clear_fifos(PROTO_NAND8_REQ_PIO, PROTO_NAND8_REQ_SM);
  pio_sm_restart(PROTO_NAND8_REQ_PIO, PROTO_NAND8_REQ_SM);
  pio_sm_clkdiv_restart(PROTO_NAND8_REQ_PIO, PROTO_NAND8_REQ_SM);
  pio_sm_set_enabled(PROTO_NAND8_REQ_PIO, PROTO_NAND8_REQ_SM, true);

  pio_sm_get_blocking(PROTO_NAND8_REQ_PIO, PROTO_NAND8_REQ_SM); //clear output fifo (discard first push)

  _proto_rsp_pio_pc = pio_add_program(PROTO_NAND8_RSP_PIO, &proto_nand8_rsp_program);
  proto_nand8_rsp_program_init(PROTO_NAND8_RSP_PIO, PROTO_NAND8_RSP_SM, _proto_rsp_pio_pc);
  pio_sm_clear_fifos(PROTO_NAND8_RSP_PIO, PROTO_NAND8_RSP_SM);
  pio_sm_restart(PROTO_NAND8_RSP_PIO, PROTO_NAND8_RSP_SM);
  pio_sm_clkdiv_restart(PROTO_NAND8_RSP_PIO, PROTO_NAND8_RSP_SM);
  pio_sm_set_enabled(PROTO_NAND8_RSP_PIO, PROTO_NAND8_RSP_SM, true);

  pio_sm_get_blocking(PROTO_NAND8_RSP_PIO, PROTO_NAND8_RSP_SM); //clear output fifo (discard first push)
}

ChipProtocolNand8::~ChipProtocolNand8(){
  _readPages = 0;
  _readCE = -1;
  safeFree(_bufPageRead);
  pio_sm_set_enabled(PROTO_NAND8_REQ_PIO, PROTO_NAND8_REQ_SM, false);
  pio_sm_clear_fifos(PROTO_NAND8_REQ_PIO, PROTO_NAND8_REQ_SM);
  pio_remove_program(PROTO_NAND8_REQ_PIO, &proto_nand8_req_program, _proto_req_pio_pc);

  pio_sm_set_enabled(PROTO_NAND8_RSP_PIO, PROTO_NAND8_RSP_SM, false);
  pio_sm_clear_fifos(PROTO_NAND8_RSP_PIO, PROTO_NAND8_RSP_SM);
  pio_remove_program(PROTO_NAND8_RSP_PIO, &proto_nand8_rsp_program, _proto_rsp_pio_pc);
}

#pragma mark private
void ChipProtocolNand8::setCEEnabled(uint8_t CE, int enabled){
  CE &= 3;
  if (CE == 0){
    gpio_put(NAND_CE0, enabled == 0);
  }else{
    gpio_put(NAND_CE1 + ((CE-1)&3), enabled == 0);
  }
}


#pragma mark public override
void ChipProtocolNand8::resetChip(){
  for (int i = 0; i < 4; i++){
    setCEEnabled(i,1);
  }

  uint8_t cmd = 0xff;
  sendCommandAddrDataRsp(0, &cmd, sizeof(cmd), NULL, 0, NULL, 0, NULL, 0);
  
  for (int i = 0; i < 4; i++){
    setCEEnabled(i,0);
  }
}

int ChipProtocolNand8::chipCommand(const void *data_, size_t dataSize, void *response, size_t responseSize, bool isMultiCommand){
  int err = 0;
  const uint8_t *data = (const uint8_t *)data_;

  uint8_t CE;
  uint16_t lenCmd;
  uint16_t lenAddr;
  uint16_t lenData;
  
  const uint8_t *bufCmd;
  const uint8_t *bufAddr;
  const uint8_t *bufData;

  cassure(dataSize>=1);
  CE = data[0];
  data++;
  dataSize--;
  
  cassure(dataSize>=sizeof(lenCmd));
  lenCmd = data[0] | data[1]<<8;
  data += 2;
  dataSize-=2;

  cassure(dataSize>=lenCmd);
  bufCmd = data;
  data += lenCmd;
  dataSize -= lenCmd;

  cassure(dataSize>=sizeof(lenAddr));
  lenAddr = data[0] | data[1]<<8;
  data += 2;
  dataSize-=2;

  cassure(dataSize>=lenAddr);
  bufAddr = data;
  data += lenAddr;
  dataSize -= lenAddr;

  cassure(dataSize>=sizeof(lenData));
  lenData = data[0] | data[1]<<8;
  data += 2;
  dataSize-=2;

  cassure(dataSize>=lenData);
  bufData = data;
  data += lenData;
  dataSize -= lenData;

  cassure(dataSize == 0); //sanity check!

  sendCommandAddrDataRsp(CE, bufCmd, lenCmd, bufAddr, lenAddr, bufData, lenData, response, responseSize, isMultiCommand);
error:
  return err;
}


int ChipProtocolNand8::prepareReadPages(const void *data_, size_t dataSize){
  int err = 0;
  const uint8_t *data = (const uint8_t *)data_;

  cassure(_readPages == 0);
  cassure(_readCE == -1);

  cassure(dataSize>=sizeof(_pageAddress));
  _pageAddress = *(uint32_t*)data;
  data+=sizeof(uint32_t);
  dataSize-=sizeof(uint32_t);

  cassure(dataSize>=sizeof(uint32_t));
  _readPages = *(uint32_t*)data;
  data+=sizeof(uint32_t);
  dataSize-=sizeof(uint32_t);

  cassure(dataSize>=sizeof(uint16_t));
  _pageSize = *(uint16_t*)data;
  data+=sizeof(uint16_t);
  dataSize-=sizeof(uint16_t);

  cassure(dataSize>=sizeof(int8_t));
  _readCE = *(int8_t*)data;
  data+=sizeof(int8_t);
  dataSize-=sizeof(int8_t);

  cassure(dataSize == 0); //sanity check!

  {
    void *nbuf = realloc(_bufPageRead, _pageSize);
    if (!nbuf){
      _pageSize = 0;
      _readCE = -1;
      creterror("allocation failure");
    }
    _bufPageRead = (uint8_t*)nbuf;
    _bufPageIndex = _pageSize;
  }

error:
  return err;
}

void ChipProtocolNand8::runTask(){
  if (_readCE != -1){
    if (_bufPageIndex < _pageSize){
      uint32_t avail = tud_vendor_write_available();
      if (avail >= 0x1000){
        uint32_t didWrite = tud_vendor_write(&_bufPageRead[_bufPageIndex], _pageSize-_bufPageIndex);
        _bufPageIndex += didWrite;
        tud_vendor_flush();
      }      
    }else if (_readPages){
      _bufPageIndex = 0;
      readPage(_readCE, _pageAddress, _pageSize, _bufPageRead);
      _pageAddress++;
      _readPages--;
    }else{
      _readCE = -1;
    }
  }else if (_readCE == -2){
    _readCE = -1;
  }
}


#pragma mark public
void ChipProtocolNand8::sendCommandAddrDataRsp(uint8_t CE, const void *cmd_, size_t cmdLen, const void *addr_, size_t addrLen, const void *data_, size_t dataLen, void *rsp_, size_t rspSize, bool isMultiCommand){
  const uint8_t *cmd = (const uint8_t *)cmd_;
  const uint8_t *addr = (const uint8_t *)addr_;
  const uint8_t *data = (const uint8_t *)data_;
  uint8_t *rsp = (uint8_t *)rsp_;

  uint32_t dummy;

  gpio_put(NAND_WP, 1);
  setCEEnabled(CE,1);

  /*
    REQ
  */

  pio_sm_put_blocking(PROTO_NAND8_REQ_PIO, PROTO_NAND8_REQ_SM, cmdLen-1);
  for (size_t i = 0; i < cmdLen; i++){
    pio_sm_put_blocking(PROTO_NAND8_REQ_PIO, PROTO_NAND8_REQ_SM, cmd[i]);
  }

  pio_sm_put_blocking(PROTO_NAND8_REQ_PIO, PROTO_NAND8_REQ_SM, addrLen);

  for (size_t i = 0; i < addrLen; i++){
    pio_sm_put_blocking(PROTO_NAND8_REQ_PIO, PROTO_NAND8_REQ_SM, addr[i]);
  }

  pio_sm_put_blocking(PROTO_NAND8_REQ_PIO, PROTO_NAND8_REQ_SM, dataLen);
  for (size_t i = 0; i < dataLen; i++){
    pio_sm_put_blocking(PROTO_NAND8_REQ_PIO, PROTO_NAND8_REQ_SM, data[i]);
  }

  dummy = pio_sm_get_blocking(PROTO_NAND8_REQ_PIO, PROTO_NAND8_REQ_SM); //wait for send command to finish

  /*
    RSP 
  */

  pio_sm_put_blocking(PROTO_NAND8_RSP_PIO, PROTO_NAND8_RSP_SM, rspSize);

  for (size_t i = 0; i < rspSize; i++){
    uint32_t rdat = pio_sm_get_blocking(PROTO_NAND8_RSP_PIO, PROTO_NAND8_RSP_SM);
    rsp[i] = rdat;
  }

  dummy = pio_sm_get_blocking(PROTO_NAND8_RSP_PIO, PROTO_NAND8_RSP_SM); //wait for receive command to finish

  if (!isMultiCommand){
    setCEEnabled(CE,0);
    gpio_put(NAND_WP, 0);
  }
}

void ChipProtocolNand8::readPage(uint8_t CE, uint32_t pageAddress, uint16_t pageSize, void *outBuf){
    uint64_t addr = pageAddress << 16;
    uint8_t cmd1 = 0x00;
    uint8_t cmd2 = 0x30;
    
    sendCommandAddrDataRsp(CE, &cmd1, sizeof(cmd1), &addr, 5, NULL, 0, NULL, 0, true);
    sendCommandAddrDataRsp(CE, &cmd2, sizeof(cmd2), NULL, 0, NULL, 0, outBuf, pageSize);
}
