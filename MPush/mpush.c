//
//  main.c
//  MPush
//
//  Created by Mao on 19/01/2017.
//  Copyright © 2017 mao. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "mao_apns_push.h"


void usage()
{
	printf("+------------------------------+\n");
	printf("+  mpush v1.0                  +\n");
	printf("+  push message to ios device. +\n");
	printf("+  author:maokebing            +\n");
	printf("+  mail:kebing1011@163.com     +\n");
	printf("+------------------------------+\n");
	printf("usage: mpush [-c cert_file(.pem) -d device_token(hex string or base64 string)] \n");
	printf("or           [-m message] \n");
	printf("or           [-b badge]   \n");
	printf("or           [-t times]   \n");
	printf("or           [-s switch to sandbox] \n");
}


const char *short_opts = "sc:d:m:b:t:";

int main(int argc, char * argv[])
{
	char *arg_cert = NULL;
	char *arg_token = NULL;
	char *arg_message = NULL;
	char *arg_badge = NULL;
	char *arg_times = NULL;
	char opt = 0;
	int  sandbox = 0;
	
	while ((opt = getopt(argc, argv, short_opts))!= -1 ) {
		switch (opt) {
			case 'c' :
				arg_cert = optarg;
				break;
			case 'd' :
				arg_token = optarg;
				break;
			case 'm' :
				arg_message = optarg;
				break;
			case 'b' :
				arg_badge = optarg;
				break;
			case 't' :
				arg_times = optarg;
				break;
			case 's' :
				sandbox = 1;
				break;
			default :
				return 1;
		}
	}
	
	//must arg
	if (!arg_cert || !arg_token)
	{
		usage();
		return 0;
	}
	
	//option arg
	if (!arg_message)
		arg_message = "This is a test message.";
	
	int badge = 1;
	if (arg_badge)
		badge = atoi(arg_badge);
	
	int times = 1;
	if (arg_times)
	{
		times = atoi(arg_times);
	}
	
	return mao_apns_push_msg(arg_cert, sandbox, arg_token, arg_message, badge, 1, times);
}
