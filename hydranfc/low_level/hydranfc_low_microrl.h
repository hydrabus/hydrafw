#ifndef HYDRANFC_LOW_MICRORL_H_INCLUDED
#define HYDRANFC_LOW_MICRORL_H_INCLUDED

extern volatile bool nfc_select_low_selected;

void hydranfc_low_print_help(t_hydra_console *con, int argc, const char* const* argv);
int hydranfc_low_execute(t_hydra_console *con, int argc, const char* const* argv);
void hydranfc_low_sigint(t_hydra_console *con);

char**          hydranfc_low_get_compl_world();
microrl_exec_t* hydranfc_low_get_keyworld();
int             hydranfc_low_get_num_of_cmd();

#endif /* HYDRANFC_LOW_MICRORL_H_INCLUDED */
