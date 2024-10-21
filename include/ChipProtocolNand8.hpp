//
//  ChipProtocolNand8.hpp
//  pico-nand-reader
//
//  Created by tihmstar on 07.10.24.
//

#ifndef ChipProtocolNand8_hpp
#define ChipProtocolNand8_hpp

#include "ChipProtocol.hpp"

#include <stdint.h>
#include <stdlib.h>


class ChipProtocolNand8 : public ChipProtocol{
  int _proto_req_pio_pc;
  int _proto_rsp_pio_pc;

  uint8_t *_bufPageRead;
  uint32_t _bufPageIndex;

  uint32_t _pageAddress;
  uint32_t _readPages;
  uint16_t _pageSize;
  int8_t  _readCE;


  void setCEEnabled(uint8_t CE, int enabled);
public:
  ChipProtocolNand8();
  virtual ~ChipProtocolNand8() override;

  virtual void resetChip() override;
  virtual int chipCommand(const void *data, size_t dataSize, void *response, size_t responseSize, bool isMultiCommand = false) override;  
  virtual int prepareReadPages(const void *data, size_t dataSize) override;
  virtual void runTask() override;

  void sendCommandAddrDataRsp(uint8_t CE, const void *cmd, size_t cmdLen, const void *addr, size_t addrLen, const void *data, size_t dataLen, void *rsp, size_t rspSize, bool isMultiCommand = false);
  void readPage(uint8_t CE, uint32_t pageAddress, uint16_t pageSize, void *outBuf);
};

#endif /* ChipProtocol_hpp */
