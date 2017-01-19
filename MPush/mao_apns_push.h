//
//  mao_apns_push.h
//  MPush
//
//  Created by Mao on 19/01/2017.
//  Copyright © 2017 mao. All rights reserved.
//

#ifndef mao_apns_push_h
#define mao_apns_push_h

#include <stdio.h>

/**
 *  push anpns msg
 *  @param cert_path pem cert path
 *  @param sandbox sandbox cert
 *  @param device_token ios device token (hex string)
 *  @param msg msg content
 *  @param badge_count badge count
 *  @param sound_on sound on: 1  off: 0
 *  @param times repeate times

 *  @return 0：sucess -1:faild
 */
int mao_apns_push_msg(const char* cert_path,
					  int sandbox,
					  const char* device_token,
					  const char* msg,
					  int badge_count,
					  int sound_on,
					  int times);


#endif /* mao_apns_push_h */
