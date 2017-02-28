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

const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int mao_apns_hexatoi(char a)
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

int base64_decode( const char * base64, unsigned char * bindata )
{
	int i, j;
	unsigned char k;
	unsigned char temp[4];
	for ( i = 0, j = 0; base64[i] != '\0' ; i += 4 )
	{
		memset( temp, 0xFF, sizeof(temp) );
		for ( k = 0 ; k < 64 ; k ++ )
		{
			if ( base64char[k] == base64[i] )
				temp[0]= k;
		}
		for ( k = 0 ; k < 64 ; k ++ )
		{
			if ( base64char[k] == base64[i+1] )
				temp[1]= k;
		}
		for ( k = 0 ; k < 64 ; k ++ )
		{
			if ( base64char[k] == base64[i+2] )
				temp[2]= k;
		}
		for ( k = 0 ; k < 64 ; k ++ )
		{
			if ( base64char[k] == base64[i+3] )
				temp[3]= k;
		}
		
		bindata[j++] = ((unsigned char)(((unsigned char)(temp[0] << 2))&0xFC)) |
		((unsigned char)((unsigned char)(temp[1]>>4)&0x03));
		if ( base64[i+2] == '=' )
			break;
		
		bindata[j++] = ((unsigned char)(((unsigned char)(temp[1] << 4))&0xF0)) |
		((unsigned char)((unsigned char)(temp[2]>>2)&0x0F));
		if ( base64[i+3] == '=' )
			break;
		
		bindata[j++] = ((unsigned char)(((unsigned char)(temp[2] << 6))&0xF0)) |
		((unsigned char)(temp[3]&0x3F));
	}
	return j;
}


int mao_apns_check_is_hex(const char* token)
{
	const char* p = token;
	while (*p) {
		if ((*p > 'f' && *p < 'z')
			|| (*p > 'F' && *p < 'Z')
			|| *p == '/'
			|| *p == '+'
			|| *p == '=')
			return 0;
		p++;
	}
	
	return 1;
}

int mao_apns_trim_string(const char* src, char* dest)
{
	if (strlen(src) > 255)
	{
		printf("device token is too large.\n");
		return -1;
	}
	
	char *p = dest;
	const char *q = src;
	
	while (*q != '\0')
	{
		if (*q != ' ')
		{
			*p++ = *q;
		}
		++q;
	}
	
	return 0;
}

void mao_apns_token_to_bytes(char* device_token, unsigned char* bytes)
{
	//to byte
	char* p = device_token;
	int i = 0;
	while (*p != '\0') {
		int h = mao_apns_hexatoi(*p++) & 0xF;
		int l = mao_apns_hexatoi(*p++) & 0xF;
		
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
	
	//trim string
	char token[255] = {0};
	if (mao_apns_trim_string(device_token, token))
		return -1;
	
	//check b64
	if (mao_apns_check_is_hex(token))
		mao_apns_token_to_bytes(token, token_bytes);
	else
		base64_decode(token, token_bytes);
	
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
		host = "gateway.sandbox.push.apple.com:2195";

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





