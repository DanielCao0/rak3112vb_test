#include "command.h"
#include "string.h"
#include <Arduino.h>


#define MAX_HANDLER_NUM 32

AT_HandlerTable handler_table[MAX_HANDLER_NUM];
int handler_table_size = 0;

// 注册函数
bool register_at_handler(const char *cmd, AT_Handler handler, const char *help) {
    if (handler_table_size >= MAX_HANDLER_NUM) return false;
    handler_table[handler_table_size].cmd = cmd;
    handler_table[handler_table_size].handler = handler;
    handler_table[handler_table_size].help = help;
    handler_table_size++;
    return true;
}

// 初始化时注册
void init_default_handlers() {
    register_at_handler("AT", handle_at, "AT Test 2025/2/14");
}


void get_all_commands()
{
    Serial.print("Available AT commands:\r\n");
    for (int i = 0; i < handler_table_size; i++)
    {
        Serial.printf("%s - %s\r\n", handler_table[i].cmd, handler_table[i].help);
    }
}

void process_serial_input(char c)
{
	static char input[MAX_CMD_LEN + MAX_PARAM_LEN + 3];
	static int i = 0;

	/* backspace */
	if (c == '\b')
	{
		if (i > 0)
		{
			i--;
			Serial.print(" \b");
		}
		return;
	}

	if (i >= MAX_CMD_LEN + MAX_PARAM_LEN + 2)
	{

		i = 0;
		Serial.print("ERROR: Input buffer overflow\r\n");
		return;
	}

	if (c == '\n' || c == '\r')
	{
		input[i] = '\0';
		if (strcasecmp(input, "AT?") == 0 || strcasecmp(input, "AT+HELP") == 0)
		{
			get_all_commands();
		}
		else
		{
			process_AT_Command(input);
		}
		i = 0;
	}
	else
	{
		input[i] = c;
		i++;
	}
}

AT_Command parse_AT_Command(const char *input)
{
	AT_Command cmd;
	const char *eq_pos = strchr(input, '=');
	if (eq_pos != NULL)
	{
		size_t cmd_len = eq_pos - input;
		/* Gets the length of the argument */
		size_t params_len = strlen(eq_pos + 1);
		memcpy(cmd.cmd, input, cmd_len);
		memcpy(cmd.params, eq_pos + 1, params_len);
		cmd.cmd[cmd_len] = '\0';
		cmd.params[params_len] = '\0';
	}
	else
	{
		/* Without the = sign, the whole string is copied */
		strcpy(cmd.cmd, input);
		cmd.params[0] = '\0';
	}

	return cmd;
}

void process_AT_Command(const char *input)
{
	AT_Command cmd = parse_AT_Command(input);

	//  start fix Hit Enter to return AT_ERROR
	if (strlen(cmd.cmd) == 0) // When the length of the AT command is 0, no processing is performed.
	{
		Serial.print("\r\n");
		return;
	}
	//  end fix Hit Enter to return AT_ERROR

	int num_handlers = sizeof(handler_table) / sizeof(handler_table[0]);
	for (int i = 0; i < num_handlers; i++)
	{
		if (strcasecmp(cmd.cmd, handler_table[i].cmd) == 0)
		{
			Serial.print("\r\n"); // Add \r\n before returning the result of the AT command.
			handler_table[i].handler(&cmd);
			return;
		}
	}
	Serial.print("AT_ERROR\r\n");
}

void handle_at(const AT_Command *cmd)
{
	Serial.print("AT\r\n");
	Serial.print("OK\r\n");
}

void command_init()
{
	init_default_handlers();
    xTaskCreate(
        atCmd,    /* Task function. */
        "atCmd",  /* String with name of task. */
        4 * 1024, /* Stack size in bytes. */
        NULL,     /* Parameter passed as input of the task */
        0,        /* Priority of the task. */
        NULL);
}


void atCmd(void *parameter)
{
	while (1)
	{
		if (Serial.available() > 0)
		{
			// 读取一个字节的数据
			char received = Serial.read();

			// 将接收到的字节写回串口
			Serial.write(received);
			process_serial_input(received);
		}

        delay(10);
	}
}