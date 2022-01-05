/*
 * defer.h
 *
 *  Created on: Nov 16, 2021
 *      Author: ovalentin
 */

#ifndef DEFER_H_
#define DEFER_H_

void defer_call(void (*fnct)(void* data), void* data);

#endif /* DEFER_H_ */
