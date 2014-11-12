#include "hydranfc_cmd_transparent.h"
#include "mcu.h"
#include "trf797x.h"
#include "types.h"
#include "tools.h"


void cmd_nfc_set_protocol(t_hydra_console *con, int argc, const char* const* argv)
{
    //tprintf("Error: cmd_nfc_set_protocol not implemented yet\r\n");
    setRF_14443_A();
}


uint32_t setRF_14443_A()
{
    int init_ms;
#undef DATA_MAX
#define DATA_MAX (20)
    uint8_t data_buf[DATA_MAX];
#undef UID_MAX
#define UID_MAX (5) /* Does not managed UID > 4+BCC to be done later ... */


    /* Test ISO14443-A/Mifare read UID */
    init_ms = Trf797xInitialSettings();
    Trf797xReset();


    /* Write Modulator and SYS_CLK Control Register (0x09) (13.56Mhz SYS_CLK and default Clock 13.56Mhz)) */
    data_buf[0] = MODULATOR_CONTROL;
    data_buf[1] = 0x31;
    Trf797xWriteSingle(data_buf, 2);

    data_buf[0] = MODULATOR_CONTROL;
    Trf797xReadSingle(data_buf, 1);

    /* Configure Mode ISO Control Register (0x01) to 0x88 (ISO14443A RX bit rate, 106 kbps) and no RX CRC (CRC is not present in the response)) */
    data_buf[0] = ISO_CONTROL;
    data_buf[1] = 0x88;
    Trf797xWriteSingle(data_buf, 2);

    data_buf[0] = ISO_CONTROL;
    Trf797xReadSingle(data_buf, 1);
    if(data_buf[0] != 0x88)
    {
      return 0xFFFFFFFF;
    }

    /* Turn RF ON (Chip Status Control Register (0x00)) */
    Trf797xTurnRfOn();

    /* Read back (Chip Status Control Register (0x00) shall be set to RF ON */
    data_buf[0] = CHIP_STATE_CONTROL;
    Trf797xReadSingle(data_buf, 1);
    return (uint32_t)data_buf[0];


}
