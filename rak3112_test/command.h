
#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>

#define MAX_CMD_LEN 64
#define MAX_PARAM_LEN 256
#define MAX_ARGV_SIZE 64

typedef struct
{
	char cmd[MAX_CMD_LEN + 1];
	char params[MAX_PARAM_LEN + 1];
	int argc;
	char argv[MAX_ARGV_SIZE][MAX_PARAM_LEN + 1];
} AT_Command;


typedef void (*AT_Handler)(const AT_Command *);

typedef struct
{
	const char *cmd;
	AT_Handler handler;
	const char *help;
} AT_HandlerTable;

void process_AT_Command(const char *input);
AT_Command parse_AT_Command(const char *input);
void process_serial_input(char c);
void get_all_commands();
void atCmd(void *parameter);
void handle_at(const AT_Command *cmd);
void command_init();
bool register_at_handler(const char *cmd, AT_Handler handler, const char *help);

#endif // COMMAND_H