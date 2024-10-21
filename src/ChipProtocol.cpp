//
//  ChipProtocol.cpp
//  pico-nand-reader
//
//  Created by tihmstar on 07.10.24.
//

#include "../include/ChipProtocol.hpp"


#pragma mark ChipProtocol
ChipProtocol::ChipProtocol(t_ChipProtocol proto)
: _proto(proto)
{
  //
}


#pragma mark public virtual
void ChipProtocol::runTask(){
  //
}

#pragma mark public
t_ChipProtocol ChipProtocol::getCurrentProtocol(){
  return _proto;
}
