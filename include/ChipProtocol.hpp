//
//  ChipProtocol.hpp
//  pico-nand-reader
//
//  Created by tihmstar on 07.10.24.
//

#ifndef ChipProtocol_hpp
#define ChipProtocol_hpp

#include "PNR-proto.h"

#include <stdlib.h>

class ChipProtocol{
  t_ChipProtocol _proto;
protected:
  ChipProtocol(t_ChipProtocol proto);

public:
  virtual ~ChipProtocol() = default;

  virtual void resetChip() = 0;
  virtual int chipCommand(const void *data, size_t dataSize, void *response, size_t responseSize, bool isMultiCommand = false) = 0;

  virtual int prepareReadPages(const void *data, size_t dataSize) = 0;

  virtual void runTask();

  t_ChipProtocol getCurrentProtocol();
};

#endif /* ChipProtocol_hpp */
