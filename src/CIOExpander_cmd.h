
#ifndef ARDUINOPROJECTMODULE_DEBUG_CIOEXPANDER_CMD_H
#define ARDUINOPROJECTMODULE_DEBUG_CIOEXPANDER_CMD_H

#define IOX_REG_INPUT_PORT0                (0x00)  // Default: XXXX XXXX, Read byte
#define IOX_REG_INPUT_PORT1                (0x01)  // Default: XXXX XXXX, Read byte
#define IOX_REG_OUTPUT_PORT0               (0x02)  // Default: 1111 1111, Read/write byte
#define IOX_REG_OUTPUT_PORT1               (0x03)  // Default: 1111 1111, Read/write byte
#define IOX_REG_POLARITY_INVERSION_PORT0   (0x04)  // Default: 0000 0000, Read/write byte            
#define IOX_REG_POLARITY_INVERSION_PORT1   (0x05)  // Default: 0000 0000, Read/write byte            
#define IOX_REG_CONFIGURATION_PORT0        (0x06)  // Default: 1111 1111, Read/write byte       
#define IOX_REG_CONFIGURATION_PORT1        (0x07)  // Default: 1111 1111, Read/write byte       

#endif //ARDUINOPROJECTMODULE_DEBUG_CIOEXPANDER_CMD_H
