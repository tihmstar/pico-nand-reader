//
//  PNR-proto.h
//  blind-nand-dumper
//
//  Created by tihmstar on 07.10.24.
//

#ifndef PNR_proto_h
#define PNR_proto_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum{
    kChipProtocolUndefined = 0,
    kChipProtocolNAND8,
    
    kChipProtocolsCount //must be last
} t_ChipProtocol;


typedef enum{
    kReaderCommandUndefined = 0,
    kReaderCommandSelectProtocol,
    kReaderCommandResetChip,
    kReaderCommandChipCommandSend,
    kReaderCommandChipCommandReceive,
    kReaderCommandChipReadPages,

    kReaderCommandsCount //must be last
} t_ReaderCommand;

typedef enum : uint8_t{
    kReaderResponseSuccess = 0,
    kReaderResponseUndefined,

    kReaderResponseUnkownFailure,
    kReaderResponseNoProtocolSelected,
    kReaderResponseAllocationFailure,
    kReaderResponseCommandFailure,
    kReaderResponseCommandReceiveSizeMismatch,
    kReaderResponseChipTimeout,
} t_ReaderResponse;


#ifdef __cplusplus
};
#endif

#endif /* PNR_proto_h */
