//
//  mao_apns_push.c
//  MPush
//
//  Created by Mao on 19/01/2017.
//  Copyright © 2017 mao. All rights reserved.
//

#include "mao_apns_push.h"

#if (defined(__WIN32__))
#include <windows.h>
#define SLEEP_ONECE Sleep(2000);
#else
#define SLEEP_ONECE sleep(2);
#include <unistd.h>
#endif

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/x509.h>


SSL_CTX                *ctx  = NULL;
SSL                    *ssl  = NULL;
const SSL_METHOD       *meth = NULL;
X509                   *cert = NULL;
EVP_PKEY               *key  = NULL;
BIO                    *bio  = NULL;


int hexatoi(char a)
{
	if (a >= '0' && a <= '9')
	{
		return a - '0';
	}
	
	if (a >= 'a' && a <= 'f')
	{
		return a - 'a' + 10;
	}
	
	if (a >= 'A' && a <= 'F')
	{
		return a - 'A' + 10;
	}
	
	return 0;
}

void mao_apns_token_to_bytes(const char* device_token, unsigned char* bytes)
{
	char token[128] = {'\0'};
	memcpy(token, device_token, strlen(device_token));
	
	//trim space
	char *p = token;
	char *q = token;

	while (*p != '\0')
	{
		if (*q != ' ')
		{
			*p++ = *q;
		}
		++q;
	}
	
	//to byte
	p = token;
	int i = 0;
	while (*p != '\0') {
		int h = hexatoi(*p++) & 0xF;
		int l = hexatoi(*p++) & 0xF;
		
		bytes[i++] = (h * 16 + l) & 0xFF;
	}
}

int mao_apns_push(const char* device_token,
				  const char* msg,
				  int badge_count,
				  int sound_on)
{
	if (strlen(msg) > 512)
	{
		printf("Msg is too large max is 512 byte\n");
	}
	
	unsigned char token_bytes[32];
	char message[1024] = {0};
	int msgLength;
	char payload[800] = {0};
    char * fmt;
	if (sound_on)
	{
		fmt = "{\"aps\":{\"alert\":\"%s\",\"badge\":%d,\"sound\":\"chime.aiff\"}}";
	}else{
		fmt = "{\"aps\":{\"alert\":\"%s\",\"badge\":%d}}";
	}
		
	sprintf(payload, fmt, msg, badge_count);
	
	mao_apns_token_to_bytes(device_token, token_bytes);
	
	unsigned char command = 0;
	size_t payloadLength = strlen(payload);
	char *pointer = message;
	unsigned short networkTokenLength = htons((u_short)32);
	unsigned short networkPayloadLength = htons((unsigned short)payloadLength);
	memcpy(pointer, &command, sizeof(unsigned char));
	pointer +=sizeof(unsigned char);
	memcpy(pointer, &networkTokenLength, sizeof(unsigned short));
	pointer += sizeof(unsigned short);
	memcpy(pointer, token_bytes, 32);
	pointer += 32;
	memcpy(pointer, &networkPayloadLength, sizeof(unsigned short));
	pointer += sizeof(unsigned short);
	memcpy(pointer, payload, payloadLength);
	pointer += payloadLength;
	msgLength = (int)(pointer - message);
	
	if (SSL_write(ssl, message, msgLength) < 0)
	{
		return -1;
	}
	
	return 0;
}

int mao_apns_init(const char* cert_path, int sandbox)
{
	char* host;
	if (sandbox)
	{
		host = "sandbox.gateway.push.apple.com:2195";

	}
	else
	{
		host= "gateway.push.apple.com:2195";
	}
	
	/*
	 * Lets get nice error messages
	 */
	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();
	
	/*
	 * Setup all the global SSL stuff
	 */
	SSL_library_init();
	
	ctx = SSL_CTX_new(SSLv23_client_method());
	if (SSL_CTX_use_certificate_chain_file(ctx, cert_path) != 1) {
		printf("load cert file faild\n");
		return -1;
	}
	if (SSL_CTX_use_PrivateKey_file(ctx, cert_path, SSL_FILETYPE_PEM) != 1) {
		printf("load key file faild\n");
		return -1;
	}
	bio = BIO_new_connect(host);
	if (!bio) {
		printf("connect gateway faild.\n");
		return -1;
	}
	if (BIO_do_connect(bio) <= 0) {
		printf("connect gateway faild.\n");
		return -1;
	}
	if (!(ssl = SSL_new(ctx))) {
		printf("create ssl faild.\n");
		return -1;
	}
	
	SSL_set_bio(ssl, bio, bio);
	
	if (SSL_connect(ssl) <= 0) {
		printf("connect ssl faild.\n");
		return -1;
	}

	return 0;
}


void mao_apns_close()
{
	if(ssl)
	{
		SSL_shutdown(ssl);
		SSL_free(ssl);
		ssl = NULL;
	}
	if(ctx)
	{
		SSL_CTX_free(ctx);
		ctx = NULL;
	}
}



int mao_apns_push_msg(const char* cert_path,
					  int sandbox,
					  const char* device_token,
					  const char* msg,
					  int badge_count,
					  int sound_on,
					  int times)
{
	if (mao_apns_init(cert_path, sandbox) < 0)
	{
		return -1;
	}
	
	if (times <= 0)
		times = 1;
	
	
	while (times--)
	{
		
		if (mao_apns_push(device_token, msg, badge_count, sound_on) < 0)
		{
			return -1;
		}
		
		printf("Push Once.\n");
		
		//sleep 2 for next push
		if (times > 0)
			SLEEP_ONECE
	}
	
	printf("Push Finish.\n");
	
	mao_apns_close();
	
	return 0;
}





