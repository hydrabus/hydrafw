#include "microrl_config.h"
#include <string.h>
#include <stdlib.h>

#include "ch.h"
#include "hal.h"

#include "common.h"
#include "microrl_common.h"
#include "microrl_callback.h"

#include "hydranfc.h"
#include "hydranfc_cmd_transparent.h"
#include "hydranfc_low_microrl.h"

#define _CMD_HELP0        "?"
#define _CMD_HELP1        "h"
#define _CMD_CLEAR        "clear"
#define _CMD_INFO         "info"

#define _CMD_NFC_LOW_EXIT0 "x"
#define _CMD_NFC_LOW_EXIT1 "exit"

void cmd_microrl_exit_nfc_low_level(t_hydra_console *con, int argc, const char* const* argv);

/* Update hydranfc_low_microrl.h => HYDRANFC_NUM_OF_CMD if new command are added/removed */
#define HYDRANFC_LOW_NUM_OF_CMD 5

microrl_exec_t hydranfc_low_keyworld[HYDRANFC_LOW_NUM_OF_CMD] = {
	/* 0  */ { _CMD_HELP0, &hydranfc_low_print_help },
	/* 1  */ { _CMD_HELP1, &hydranfc_low_print_help },
	/* 2  */ { _CMD_CLEAR, &print_clear },
	/* 2  */ { _CMD_NFC_LOW_EXIT0, &cmd_microrl_exit_nfc_low_level },
	/* 2  */ { _CMD_NFC_LOW_EXIT1, &cmd_microrl_exit_nfc_low_level },
///* 3  */ { _CMD_NFC_PROTOCOL,&cmd_nfc_low_level }
};

// array for completion
char* hydranfc_low_compl_world[HYDRANFC_LOW_NUM_OF_CMD + 1];
volatile bool nfc_select_low_selected = FALSE;

char** hydranfc_low_get_compl_world()
{
	return hydranfc_low_compl_world;
}

microrl_exec_t* hydranfc_low_get_keyworld()
{
	return &hydranfc_low_keyworld[0];
}

int hydranfc_low_get_num_of_cmd()
{
	return HYDRANFC_LOW_NUM_OF_CMD;
}
//*****************************************************************************
void hydranfc_low_print_help(t_hydra_console *con, int argc, const char* const* argv)
{
	(void)argc;
	(void)argv;

	print(con, "Low level NFC API - See C# library: \n\r");
	print(con, "Use TAB key for completion\n\r");
	print(con, "? or h         - Help\t\n\r");
	print(con, "x or exit      - exit the Low Level NFC API menu\n\r");
	print(con, "clear          - clear screen\n\r");
	//print(con, _CMD_NFC_PROTOCOL " <protocol> - NFC Select protocol \n\r");

}

//*****************************************************************************
// execute callback for microrl library
// do what you want here, but don't write to argv!!! read only!!
int hydranfc_low_execute(t_hydra_console *con, int argc, const char* const* argv)
{
	bool cmd_found;
	int curr_arg = 0;
	int cmd;

	// just iterate through argv word and compare it with your commands
	cmd_found = FALSE;
	curr_arg = 0;
	while(curr_arg < argc) {
		cmd=0;
		while(cmd < HYDRANFC_LOW_NUM_OF_CMD) {
			if( (strcmp(argv[curr_arg], hydranfc_low_keyworld[cmd].str_cmd)) == 0 ) {
				hydranfc_low_keyworld[cmd].ptFunc_exe_cmd(con, argc-curr_arg, &argv[curr_arg]);
				cmd_found = TRUE;
				break;
			}
			cmd++;
		}
		if(cmd_found == FALSE) {
			print(con,"command: '");
			print(con,(char*)argv[curr_arg]);
			print(con,"' Not found.\n\r");
		}
		curr_arg++;
	}
	return 0;
}

//*****************************************************************************
void hydranfc_low_sigint(t_hydra_console *con)
{
	print(con, "HydraNFC Low Level microrl ^C catched!\n\r");
}

void cmd_microrl_exit_nfc_low_level(t_hydra_console *con, int argc, const char* const* argv)
{
	(void)argc;
	(void)argv;
	cprintf(con, "Leaving NFC Low level mode\r\n");
	nfc_select_low_selected = FALSE;
}
