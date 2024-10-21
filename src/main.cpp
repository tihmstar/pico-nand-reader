

#include "../include/ChipProtocol.hpp"
#include "../include/ChipProtocolNand8.hpp"

#include "../include/macros.h"


#include <stdio.h>
#include <tusb.h>
#include "pico/stdlib.h"


static ChipProtocol *gChipProto = nullptr;

static uint8_t *gDataBuf = NULL;
static size_t gDataBufSize = 0;

static uint8_t *gRspBuf = NULL;
static size_t gRspBufSize = 0;
static size_t gRspBufReadIndex = 0;

t_ReaderResponse gLastCommandErr = kReaderResponseUndefined;

extern "C" bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request){
  int err = 0;
  t_ReaderResponse rsp = kReaderResponseUndefined;

  switch (request->bRequest){
    case kReaderCommandSelectProtocol:
      if (stage != CONTROL_STAGE_SETUP) return true;
      {
        safeDelete(gChipProto);
        switch (request->wIndex){
          case kChipProtocolNAND8:
            gChipProto = new ChipProtocolNand8();
            break;
          
          default:
            creterror("bad protocol %d",request->wIndex);
        }
      }
      break;
    
    case kReaderCommandResetChip:
      if (stage != CONTROL_STAGE_SETUP) return true;
      gChipProto->resetChip();
      break;

    case kReaderCommandChipCommandSend:
    {
      if (stage == CONTROL_STAGE_SETUP){
        uint8_t *nRspBuf = (uint8_t*)realloc(gRspBuf, (gRspBufSize = request->wValue));
        uint8_t *nDataBuf = (uint8_t*)realloc(gDataBuf, gDataBufSize = request->wLength);
        if (!nRspBuf || !nDataBuf){
          gRspBufSize = 0;
          gDataBufSize = 0;
          gLastCommandErr = kReaderResponseAllocationFailure;
          return true;
        }
        gRspBuf = nRspBuf;
        gDataBuf = nDataBuf;
        gRspBufReadIndex = 0;
        return tud_control_xfer(rhport, request, gDataBuf, gDataBufSize);
      }
      if (stage == CONTROL_STAGE_DATA){
        int cmderr = gChipProto->chipCommand(gDataBuf, gDataBufSize, gRspBuf, gRspBufSize, request->wIndex);
        if (cmderr){
          gLastCommandErr = kReaderResponseCommandFailure;
          gRspBufSize = 0;
          gRspBufReadIndex = 0;
        }else{
          gLastCommandErr = kReaderResponseSuccess;
        }
      }
      return true;
    }
      break;

    case kReaderCommandChipCommandReceive:
    {
      if (stage == CONTROL_STAGE_SETUP){
        size_t actualSize = MIN(gRspBufSize-gRspBufReadIndex,request->wLength);
        bool ret = tud_control_xfer(rhport, request, &gRspBuf[gRspBufReadIndex], actualSize);
        gRspBufReadIndex += actualSize;
        return ret;
      }
      return true;
    }
      break;

    case kReaderCommandChipReadPages:
    {
      if (stage == CONTROL_STAGE_SETUP){
        uint8_t *nDataBuf = (uint8_t*)realloc(gDataBuf, gDataBufSize = request->wLength);
        if (!nDataBuf){
          gDataBufSize = 0;
          gLastCommandErr = kReaderResponseAllocationFailure;
          return true;
        }
        gDataBuf = nDataBuf;
        return tud_control_xfer(rhport, request, gDataBuf, gDataBufSize);
      }
      if (stage == CONTROL_STAGE_DATA){
        int cmderr = gChipProto->prepareReadPages(gDataBuf, gDataBufSize);
        if (cmderr){
          gLastCommandErr = kReaderResponseCommandFailure;
          gRspBufSize = 0;
          gRspBufReadIndex = 0;
        }else{
          gLastCommandErr = kReaderResponseSuccess;
        }
      }
      return true;
    }
      break;


    default:
      creterror("bad request %d",request->wIndex);
  }

  rsp = kReaderResponseSuccess;
error:
  if (err){
    if (rsp == kReaderResponseUndefined){
      rsp = kReaderResponseUnkownFailure;
    }
  }
  return tud_control_xfer(rhport, request, &rsp, sizeof(rsp));
}

void pnr_task(){
  if (!gChipProto) return;
  gChipProto->runTask();
}

int main() {
  tusb_init();

  while (1){
    tud_task(); // tinyusb device task
    pnr_task();
  }

  while (1);
  return 0;
}